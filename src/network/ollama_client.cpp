// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements stackless awaitable loops with chunk parsing boundaries
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "network/ollama_client.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/cobalt/generator.hpp>
#include <format>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "common/constants.hpp"
#include "network/ollama_requests.hpp"

namespace malama::network {

OllamaClient::OllamaClient(std::string host_name, std::string port_number) noexcept
    : m_socket(m_io_context),
      m_host(std::move(host_name)),
      m_port(std::move(port_number)),
      m_work_guard(boost::asio::make_work_guard(m_io_context)),
      m_worker_thread([this]() { m_io_context.run(); }) {}

OllamaClient::~OllamaClient() noexcept {
    m_work_guard.reset();
    m_io_context.stop();
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
    boost::system::error_code error_code;
    m_socket.close(error_code);
}

auto OllamaClient::GetExecutor() noexcept -> boost::asio::io_context::executor_type {
    return m_io_context.get_executor();
}

void OllamaClient::TriggerActiveGenerationCancellation() noexcept {
    m_cancellation_requested.store(true);
    boost::system::error_code ignore_error;
    m_socket.close(ignore_error);
}

auto OllamaClient::CheckCache(const std::string &cache_key) noexcept -> std::optional<std::string> {
    auto iterator = m_cache_store.m_lookup_table.find(cache_key);
    if (iterator != m_cache_store.m_lookup_table.end()) {
        spdlog::info("In-memory cache hit achieved for prompt signature context.");
        return iterator->second;
    }
    return std::nullopt;
}

void OllamaClient::UpdateCache(const std::string &cache_key,
                               const std::string &response_value) noexcept {
    if (m_cache_store.m_lookup_table.size() >= ResponseCache::max_cache_entries) {
        std::string oldest_key = m_cache_store.m_access_order.front();
        m_cache_store.m_lookup_table.erase(oldest_key);
        m_cache_store.m_access_order.erase(m_cache_store.m_access_order.begin());
    }
    m_cache_store.m_lookup_table[cache_key] = response_value;
    m_cache_store.m_access_order.push_back(cache_key);
}

auto OllamaClient::PrepareHttpRequest(std::string model_name,
                                      const std::vector<core::Message> &history,
                                      std::string prompt_text,
                                      const common::SessionParameters &params,
                                      std::vector<std::string> images_payload) const noexcept
    -> boost::beast::http::request<boost::beast::http::string_body> {
    std::vector<ChatMessagePayload> chat_messages;

    if (!params.m_system_prompt.empty()) {
        chat_messages.push_back(ChatMessagePayload{
            .m_role = "system", .m_content = params.m_system_prompt, .m_images = {}});
    }

    // Replay prior turns so the model has conversational context.
    for (const auto &msg : history) {
        std::string role;
        switch (msg.m_role) {
            case core::MessageRole::User:
                role = "user";
                break;
            case core::MessageRole::Assistant:
                role = "assistant";
                break;
            case core::MessageRole::System:
                role = "system";
                break;
        }
        chat_messages.push_back(ChatMessagePayload{
            .m_role = std::move(role), .m_content = msg.m_content, .m_images = {}});
    }

    chat_messages.push_back(ChatMessagePayload{.m_role = "user",
                                               .m_content = std::move(prompt_text),
                                               .m_images = std::move(images_payload)});

    OllamaChatRequest payload{.m_model = std::move(model_name),
                              .m_messages = std::move(chat_messages),
                              .m_stream = true,
                              .m_options = {.m_temperature = params.m_temperature,
                                            .m_top_p = params.m_top_p,
                                            .m_top_k = params.m_top_k,
                                            .m_repeat_penalty = params.m_repeat_penalty,
                                            .m_num_ctx = params.m_num_ctx}};

    std::string json_body;
    [[maybe_unused]] const auto write_error = glz::write_json(payload, json_body);

    boost::beast::http::request<boost::beast::http::string_body> request_packet{
        boost::beast::http::verb::post, "/api/chat", constants::http_version_1_1};
    request_packet.set(boost::beast::http::field::host, m_host);
    request_packet.set(boost::beast::http::field::content_type, "application/json");
    request_packet.body() = std::move(json_body);
    request_packet.prepare_payload();

    return request_packet;
}

void OllamaClient::ParseStreamAccumulator(
    const std::function<void(std::string_view)> &token_callback,
    std::string &complete_stream_accumulation) noexcept {
    std::size_t newline_position = 0;
    while ((newline_position = m_residual_line_accumulator.find('\n')) != std::string::npos) {
        std::string_view zero_copy_line_view(m_residual_line_accumulator.data(), newline_position);

        if (!zero_copy_line_view.empty() && zero_copy_line_view.front() == '{') {
            ResponseChunk chunk;
            auto parser_error =
                glz::read<glz::opts{.error_on_unknown_keys = false}>(chunk, zero_copy_line_view);

            if (!parser_error && token_callback) {
                token_callback(chunk.m_message.m_content);
                complete_stream_accumulation += chunk.m_message.m_content;
            }
        }
        m_residual_line_accumulator.erase(0, newline_position + 1);
    }
}

auto OllamaClient::ExecuteStreamTask(std::string model_name,
                                     const std::vector<core::Message> &history,
                                     std::string prompt_text,
                                     std::vector<std::string> images_payload,
                                     std::function<void(std::string_view)> token_callback) noexcept
    -> boost::cobalt::task<std::expected<void, common::NetworkError>> {
    co_return co_await ExecuteStreamTask(std::move(model_name), history, std::move(prompt_text),
                                         common::SessionParameters{}, std::move(images_payload),
                                         std::move(token_callback));
}

auto OllamaClient::ExecuteStreamTask(std::string model_name,
                                     const std::vector<core::Message> &history,
                                     std::string prompt_text, common::SessionParameters params,
                                     std::vector<std::string> images_payload,
                                     std::function<void(std::string_view)> token_callback) noexcept
    -> boost::cobalt::task<std::expected<void, common::NetworkError>> {
    if (m_is_streaming.load()) {
        co_return std::unexpected(common::NetworkError::HostUnreachable);
    }

    m_cancellation_requested.store(false);
    m_residual_line_accumulator.clear();

    const std::string cache_key =
        std::format("{}:{}:{}", model_name, prompt_text, params.m_system_prompt);
    if (auto cached_response = CheckCache(cache_key); cached_response.has_value()) {
        if (token_callback) {
            token_callback(cached_response.value());
        }
        co_return std::expected<void, common::NetworkError>{};
    }

    m_is_streaming.store(true);
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::ip::tcp::resolver resolver(executor);

    auto endpoints_result = co_await resolver.async_resolve(m_host, m_port, boost::asio::as_tuple);
    if (std::get<0>(endpoints_result)) {
        m_is_streaming.store(false);
        co_return std::unexpected(common::NetworkError::HostUnreachable);
    }

    boost::beast::tcp_stream stream(executor);
    co_await stream.async_connect(std::get<1>(endpoints_result), boost::asio::as_tuple);

    auto request_packet =
        PrepareHttpRequest(model_name, history, prompt_text, params, std::move(images_payload));
    co_await boost::beast::http::async_write(stream, request_packet, boost::asio::as_tuple);

    boost::beast::flat_buffer static_buffer;
    boost::beast::http::response_parser<boost::beast::http::string_body> response_parser;
    response_parser.body_limit(std::numeric_limits<std::uint64_t>::max());

    co_await boost::beast::http::async_read_header(stream, static_buffer, response_parser,
                                                   boost::asio::as_tuple);

    std::string complete_stream_accumulation;
    while (!response_parser.is_done()) {
        if (m_cancellation_requested.load()) {
            if (token_callback) {
                token_callback("\n\n[Prompt generation stopped by the user.]");
            }
            stream.close();
            break;
        }

        auto [read_error, bytes_transferred] = co_await boost::beast::http::async_read_some(
            stream, static_buffer, response_parser, boost::asio::as_tuple);

        if (bytes_transferred > 0) {
            auto &body_chunk = response_parser.get().body();
            if (!body_chunk.empty()) {
                m_residual_line_accumulator.append(body_chunk);
                body_chunk.clear();
                ParseStreamAccumulator(token_callback, complete_stream_accumulation);
            }
        }

        if (read_error == boost::beast::http::error::end_of_stream || read_error) {
            break;
        }
    }

    if (!complete_stream_accumulation.empty() && !m_cancellation_requested.load()) {
        UpdateCache(cache_key, complete_stream_accumulation);
    }

    m_is_streaming.store(false);
    co_return std::expected<void, common::NetworkError>{};
}

}  // namespace malama::network
