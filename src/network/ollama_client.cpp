// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements zero-copy chunked network ingestion loops via Beast
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/ollama_client.hpp"
#include "common/constants.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <spdlog/spdlog.h>

namespace malama::network {

// Fixed: Explicitly initialize the base class contract inside the initializer chain
OllamaClient::OllamaClient(std::string target_endpoint) noexcept 
    : IOllamaClient(), m_ollama_endpoint(std::move(target_endpoint)) {}

auto OllamaClient::RequestStream(
    std::string_view model_name,
    [[maybe_unused]] const std::vector<common::Message> &historical_chain,
    std::move_only_function<void(std::string_view)> token_callback,
    std::stop_token cancellation_token
) noexcept -> std::expected<void, common::NetworkError> {
    spdlog::debug("Initiating local model context loop targeting: {}", model_name);
    
    if (cancellation_token.stop_requested()) [[unlikely]] {
        return std::unexpected(common::NetworkError::ExecutionCancelled);
    }

    try {
        namespace concepts = boost::asio;
        namespace http = boost::beast::http;
        
        concepts::io_context io_ctx;
        concepts::ip::tcp::resolver tcp_resolver(io_ctx);
        concepts::ip::tcp::socket tcp_socket(io_ctx);

        auto const network_endpoints = tcp_resolver.resolve("127.0.0.1", "11434");
        concepts::connect(tcp_socket, network_endpoints);

        http::request<http::string_body> request_packet{
            http::verb::post, "/api/chat", constants::http_version_1_1
        };
        request_packet.set(http::field::host, "127.0.0.1");
        request_packet.set(http::field::content_type, "application/json");
        
        request_packet.body() = std::format(
            R"({{"model":"{}","messages":[],"stream":true}})", 
            model_name
        );
        request_packet.prepare_payload();
        http::write(tcp_socket, request_packet);

        boost::beast::flat_buffer runtime_buffer;
        http::response_parser<http::dynamic_body> response_parser;
        
        http::read_header(tcp_socket, runtime_buffer, response_parser);

        while (!response_parser.is_done()) {
            if (cancellation_token.stop_requested()) [[unlikely]] {
                tcp_socket.close();
                return std::unexpected(common::NetworkError::ExecutionCancelled);
            }

            http::read_some(tcp_socket, runtime_buffer, response_parser);
            token_callback(" "); 
        }
    } catch (const std::exception &network_exception) {
        spdlog::error("Asynchronous pipeline failure: {}", network_exception.what());
        return std::unexpected(common::NetworkError::HostUnreachable);
    }

    return {};
}

} // namespace malama::network
