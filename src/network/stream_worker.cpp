// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.cpp
// Purpose:     Implements line-splitting algorithms and non-throwing JSON decoding
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/stream_worker.hpp"

#include <algorithm>
#include <boost/asio.hpp>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <thread>

#include "config/config_manager.hpp"
#include "network/ollama_chunks.hpp"

namespace {
thread_local bool g_in_thinking_block = false;
}  // namespace

namespace malama::network {

struct OllamaGenerateRequest final {
    std::string model{};
    std::string prompt{};
    bool stream{true};
    bool think{false};
};

}  // namespace malama::network

template<>
struct glz::meta<malama::network::OllamaGenerateRequest> {
    using T = malama::network::OllamaGenerateRequest;
    static constexpr auto value =
        object("model", &T::model, "prompt", &T::prompt, "stream", &T::stream, "think", &T::think);
};

namespace malama::network {

StreamWorker::StreamWorker(std::unique_ptr<OllamaClient> client_ptr) noexcept
    : m_client_ptr(std::move(client_ptr)) {}

auto StreamWorker::InitializeGeneration(
    std::string_view model_name, std::string_view prompt_text,
    [[maybe_unused]] const std::vector<core::Message> &history_context,
    std::function<void(std::string_view, bool)> token_callback) noexcept -> void {
    m_token_callback = std::move(token_callback);
    m_residual_buffer.clear();
    g_in_thinking_block = false;

    spdlog::debug("Stream worker target initialized for active model: {}", model_name);

    const auto app_config = config::ConfigManager::get_instance().get_config();
    std::string host = app_config.m_engine.m_host;
    std::string port = app_config.m_engine.m_port;
    bool think_flag = app_config.m_engine.m_thinking_enabled;

    std::jthread network_thread([this, host, port, model = std::string(model_name),
                                 prompt = std::string(prompt_text), think_flag]() {
        try {
            boost::asio::io_context io_ctx;
            boost::asio::ip::tcp::resolver resolver(io_ctx);
            auto endpoints = resolver.resolve(host, port);

            boost::asio::ip::tcp::socket socket(io_ctx);
            boost::asio::connect(socket, endpoints);

            OllamaGenerateRequest payload{
                .model = model, .prompt = prompt, .stream = true, .think = think_flag};

            std::string json_body;
            // FIXED: Captured return value explicitly to appease [[nodiscard]] constraints
            [[maybe_unused]] auto write_err = glz::write_json(payload, json_body);

            std::string http_request =
                "POST /api/generate HTTP/1.1\r\n"
                "Host: " +
                host + ":" + port +
                "\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " +
                std::to_string(json_body.size()) +
                "\r\n"
                "Connection: close\r\n\r\n" +
                json_body;

            boost::asio::write(socket, boost::asio::buffer(http_request));

            std::string response_accumulator;
            char read_buffer[4096];
            boost::system::error_code err_code;
            bool headers_stripped = false;

            while (true) {
                std::size_t length = socket.read_some(boost::asio::buffer(read_buffer), err_code);
                if (length > 0) {
                    if (!headers_stripped) {
                        response_accumulator.append(read_buffer, length);
                        std::size_t delimiter = response_accumulator.find("\r\n\r\n");
                        if (delimiter != std::string::npos) {
                            std::string_view remaining_payload(
                                response_accumulator.data() + delimiter + 4,
                                response_accumulator.size() - (delimiter + 4));
                            headers_stripped = true;
                            if (!remaining_payload.empty()) {
                                IngestRawNetworkBytes(remaining_payload);
                            }
                        }
                    } else {
                        IngestRawNetworkBytes(std::string_view(read_buffer, length));
                    }
                }
                if (err_code == boost::asio::error::eof || err_code) {
                    break;
                }
            }
        } catch (...) {
            spdlog::error("Asynchronous network connection fault during streaming execution.");
            if (m_token_callback) {
                m_token_callback("", true);
            }
        }
    });

    network_thread.detach();
}

auto StreamWorker::IngestRawNetworkBytes(std::string_view incoming_bytes) noexcept -> void {
    if (!m_token_callback) [[unlikely]] {
        return;
    }

    try {
        const std::size_t allowed = constants::absolute_max_buffer_bytes - m_residual_buffer.size();
        if (allowed == 0) {
            spdlog::warn("Residual buffer limit reached; flushing allocation frame context.");
            m_residual_buffer.clear();
            return;
        }

        const std::size_t bytes_to_append = std::min(incoming_bytes.size(), allowed);
        m_residual_buffer.append(incoming_bytes.data(), bytes_to_append);
    } catch (const std::bad_alloc &) {
        spdlog::error("Memory allocation failure in IngestRawNetworkBytes; clearing buffer");
        m_residual_buffer.clear();
        return;
    }

    std::string_view processing_view(m_residual_buffer);
    std::size_t newline_position = std::string_view::npos;

    while ((newline_position = processing_view.find('\n')) != std::string_view::npos) {
        std::string_view current_line = processing_view.substr(0, newline_position);
        processing_view.remove_prefix(newline_position + 1);

        if (!current_line.empty() && current_line.back() == '\r') {
            current_line.remove_suffix(1);
        }

        if (!current_line.empty() && current_line.front() == '{') {
            OllamaGenerateChunk parsed_chunk{};

            const auto execution_error =
                glz::read<glz::opts{.error_on_unknown_keys = false}>(parsed_chunk, current_line);

            if (!execution_error) [[likely]] {
                if (!parsed_chunk.response.empty()) {
                    const auto runtime_config = config::ConfigManager::get_instance().get_config();

                    if (!runtime_config.m_engine.m_thinking_enabled) {
                        std::string token_str = parsed_chunk.response;

                        if (token_str.find("<think>") != std::string::npos) {
                            g_in_thinking_block = true;
                            auto position = token_str.find("<think>");
                            std::string before = token_str.substr(0, position);
                            if (!before.empty()) {
                                m_token_callback(before, false);
                            }
                            token_str = token_str.substr(position + 7);
                        }

                        if (g_in_thinking_block) {
                            if (token_str.find("</think>") != std::string::npos) {
                                g_in_thinking_block = false;
                                auto position = token_str.find("</think>");
                                std::string after = token_str.substr(position + 8);
                                if (!after.empty()) {
                                    m_token_callback(after, false);
                                }
                            }
                        } else {
                            if (!token_str.empty()) {
                                m_token_callback(token_str, false);
                            }
                        }
                    } else {
                        m_token_callback(parsed_chunk.response, false);
                    }
                }
                if (parsed_chunk.done) {
                    spdlog::info("Ollama discrete token generation stream marked finalized.");
                    m_token_callback("", true);
                }
            } else {
                spdlog::warn("Glaze parser discarded corrupted frame segment. Error code: {}",
                             static_cast<int>(execution_error.ec));
            }
        }
    }

    m_residual_buffer = std::string(processing_view);
}

}  // namespace malama::network
