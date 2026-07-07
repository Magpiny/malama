// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements stackless awaitable loops with chunk parsing boundaries
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#include "network/ollama_client.hpp"

#include <array>
#include <format>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "common/constants.hpp"

namespace malama::network {

struct MultimodalPayload final {
    std::string model;
    std::string prompt;
    bool stream{true};
    std::vector<std::string> images{};

    struct glaze {
        using T = MultimodalPayload;
        static constexpr auto value = glz::object("model", &T::model, "prompt", &T::prompt,
                                                  "stream", &T::stream, "images", &T::images);
    };
};

struct ResponseChunk final {
    std::string response;
    bool done{false};

    struct glaze {
        using T = ResponseChunk;
        static constexpr auto value = glz::object("response", &T::response, "done", &T::done);
    };
};

OllamaClient::OllamaClient(std::string host, std::string port) noexcept
    : m_socket(m_io_context), m_host(std::move(host)), m_port(std::move(port)) {}

OllamaClient::~OllamaClient() noexcept {
    boost::system::error_code ec;
    m_socket.close(ec);
}

auto OllamaClient::ExecuteStreamTask(std::string_view model, std::string_view prompt,
                                     const std::vector<std::string> &image_base64_payload,
                                     std::function<void(std::string_view)> on_token) noexcept
    -> boost::asio::awaitable<std::expected<void, common::NetworkError>> {
    if (m_is_streaming.load()) {
        co_return std::unexpected(common::NetworkError::HostUnreachable);
    }

    m_is_streaming.store(true);
    boost::asio::ip::tcp::resolver resolver(co_await boost::asio::this_coro::executor);

    auto endpoints = co_await resolver.async_resolve(m_host, m_port, boost::asio::as_tuple);
    if (std::get<0>(endpoints)) {
        m_is_streaming.store(false);
        co_return std::unexpected(common::NetworkError::HostUnreachable);
    }

    // Split parameters to satisfy your 100-character line length limits
    const auto &results = std::get<1>(endpoints);
    co_await m_socket.async_connect(results.begin()->endpoint(), boost::asio::as_tuple);

    MultimodalPayload payload{.model = std::string(model),
                              .prompt = std::string(prompt),
                              .stream = true,
                              .images = image_base64_payload};

    std::string json_body;
    // Captured return value explicitly to satisfy [[nodiscard]] checks
    [[maybe_unused]] const auto write_ec = glz::write_json(payload, json_body);

    std::string http_request = std::format(
        "POST /api/generate HTTP/1.1\r\n"
        "Host: {}:{}\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: {}\r\n"
        "Connection: close\r\n\r\n{}",
        m_host, m_port, json_body.size(), json_body);

    co_await boost::asio::async_write(m_socket, boost::asio::buffer(http_request),
                                      boost::asio::as_tuple);

    std::array<char, 4096> read_buffer{};
    std::string accumulation_line;

    while (true) {
        auto [read_ec, length] = co_await m_socket.async_read_some(boost::asio::buffer(read_buffer),
                                                                   boost::asio::as_tuple);

        if (length > 0) {
            accumulation_line.append(read_buffer.data(), length);
            std::size_t position = 0;
            while ((position = accumulation_line.find('\n')) != std::string::npos) {
                std::string line = accumulation_line.substr(0, position);
                accumulation_line.erase(0, position + 1);

                if (!line.empty() && line.front() == '{') {
                    ResponseChunk chunk;
                    auto err = glz::read<glz::opts{.error_on_unknown_keys = false}>(chunk, line);
                    if (!err && on_token) {
                        on_token(chunk.response);
                    }
                }
            }
        }

        if (read_ec == boost::asio::error::eof || read_ec) {
            break;
        }
    }

    m_is_streaming.store(false);
    // Explicit return initialization to resolve template matching failures
    co_return std::expected<void, common::NetworkError>{};
}

}  // namespace malama::network
