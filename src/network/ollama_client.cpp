// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements stackless awaitable loops with chunk parsing boundaries
// Author:      Wanjare <wanpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-V3-or-later
// /////////////////////////////////////////////////////////////////////////////

#include "network/ollama_client.hpp"

#include <algorithm>
#include <array>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/cobalt/generator.hpp>
#include <format>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "common/constants.hpp"

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

auto OllamaClient::UpdateCache(const std::string &cache_key,
                               const std::string &response_value) noexcept -> void {
    if (m_cache_store.m_lookup_table.size() >= ResponseCache::max_cache_entries) {
        std::string oldest_key = m_cache_store.m_access_order.front();
        m_cache_store.m_lookup_table.erase(oldest_key);
        m_cache_store.m_access_order.erase(m_cache_store.m_access_order.begin());
    }
    m_cache_store.m_lookup_table[cache_key] = response_value;
    m_cache_store.m_access_order.push_back(cache_key);
}

auto OllamaClient::ExecuteStreamTask(std::string_view model_name, std::string_view prompt_text,
                                     const std::vector<std::string> &images_payload,
                                     std::function<void(std::string_view)> token_callback) noexcept
    -> boost::cobalt::task<std::expected<void, common::NetworkError>> {
    if (m_is_streaming.load()) {
        co_return std::unexpected(common::NetworkError::HostUnreachable);
    }

    m_cancellation_requested.store(false);
    m_residual_line_accumulator.clear();

    std::string cache_key = std::format("{}:{}", model_name, prompt_text);
    auto cached_response = CheckCache(cache_key);
    if (cached_response.has_value() && token_callback) {
        token_callback(cached_response.value());
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

    MultimodalPayload payload{.m_model = std::string(model_name),
                              .m_prompt = std::string(prompt_text),
                              .m_stream = true,
                              .m_images = images_payload};

    std::string json_body;
    [[maybe_unused]] const auto write_error = glz::write_json(payload, json_body);

    boost::beast::http::request<boost::beast::http::string_body> request_packet{
        boost::beast::http::verb::post, "/api/generate", 11};
    request_packet.set(boost::beast::http::field::host, m_host);
    request_packet.set(boost::beast::http::field::content_type, "application/json");
    request_packet.body() = json_body;
    request_packet.prepare_payload();

    co_await boost::beast::http::async_write(stream, request_packet, boost::asio::as_tuple);

    boost::beast::flat_buffer static_buffer;
    boost::beast::http::response_parser<boost::beast::http::string_body> response_parser;
    response_parser.body_limit(std::numeric_limits<std::uint64_t>::max());

    co_await boost::beast::http::async_read_header(stream, static_buffer, response_parser,
                                                   boost::asio::as_tuple);

    std::string complete_stream_accumulation;
    while (!response_parser.is_done()) {
        // Intercept loops securely if user requests cancellation
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
                // FIXED: Append incoming frames directly to our loopahead buffer
                m_residual_line_accumulator.append(body_chunk);
                body_chunk.clear();

                std::size_t newline_position = 0;
                while ((newline_position = m_residual_line_accumulator.find('\n')) !=
                       std::string::npos) {
                    // Create zero-copy views to parse lines without temporary string copies
                    std::string_view zero_copy_line_view(m_residual_line_accumulator.data(),
                                                         newline_position);

                    if (!zero_copy_line_view.empty() && zero_copy_line_view.front() == '{') {
                        ResponseChunk chunk;
                        auto parser_error = glz::read<glz::opts{.error_on_unknown_keys = false}>(
                            chunk, zero_copy_line_view);

                        if (!parser_error && token_callback) {
                            token_callback(chunk.m_response);
                            complete_stream_accumulation += chunk.m_response;
                        }
                    }
                    m_residual_line_accumulator.erase(0, newline_position + 1);
                }
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
