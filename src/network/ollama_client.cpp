// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements the Boost.Asio non-blocking socket state machine
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/ollama_client.hpp"
#include "network/ollama_requests.hpp"
#include "common/constants.hpp"

#include <format>
#include <spdlog/spdlog.h>

namespace malama::network {

OllamaClient::OllamaClient(std::string host, std::string port) noexcept
    : m_resolver(m_io_context), m_socket(m_io_context), m_host(std::move(host)), m_port(std::move(port)) {}

OllamaClient::~OllamaClient() noexcept {
    boost::system::error_code ec;
    m_socket.cancel(ec);
    m_socket.close(ec);
    m_io_context.stop();
}

auto OllamaClient::SubmitPrompt(std::string_view prompt_text, std::string_view model_name, std::function<void(std::string_view)> on_data) noexcept -> void {
    m_on_data_callback = std::move(on_data);

    OllamaGenerateRequest payload{
        .model = std::string(model_name),
        .prompt = std::string(prompt_text),
        .stream = true
    };

    std::string json_body;
    
    if (const auto parse_error = glz::write_json(payload, json_body); parse_error) {
        spdlog::error("Failed to serialize outbound JSON payload. Error flag: {}", static_cast<int>(parse_error.ec));
        return;
    }

    // Fixed: Downgraded to HTTP/1.0 to eliminate Transfer-Encoding: chunked hex pollution
    m_request_buffer = std::format(
        "POST {} HTTP/1.0\r\n"
        "Host: {}:{}\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: {}\r\n\r\n"
        "{}",
        constants::ollama_generate_path, m_host, m_port, json_body.size(), json_body
    );

    m_io_context.restart();
    DoResolve();

    m_context_thread = std::jthread([this]() {
        m_io_context.run();
    });
}

auto OllamaClient::DoResolve() noexcept -> void {
    m_resolver.async_resolve(m_host, m_port, [this](boost::system::error_code ec, const boost::asio::ip::tcp::resolver::results_type &endpoints) {
        if (!ec) [[likely]] {
            DoConnect(endpoints);
        } else {
            spdlog::error("Socket DNS resolution failure: {}", ec.message());
        }
    });
}

auto OllamaClient::DoConnect(const boost::asio::ip::tcp::resolver::results_type &endpoints) noexcept -> void {
    boost::asio::async_connect(m_socket, endpoints, [this](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint &/*endpoint*/) {
        if (!ec) [[likely]] {
            DoWrite();
        } else {
            spdlog::error("TCP Socket connection refused: {}", ec.message());
        }
    });
}

auto OllamaClient::DoWrite() noexcept -> void {
    boost::asio::async_write(m_socket, boost::asio::buffer(m_request_buffer), [this](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
        if (!ec) [[likely]] {
            DoRead();
        } else {
            spdlog::error("TCP Socket payload transmission fault: {}", ec.message());
        }
    });
}

auto OllamaClient::DoRead() noexcept -> void {
    m_socket.async_read_some(boost::asio::buffer(m_read_buffer), [this](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (!ec) [[likely]] {
            if (m_on_data_callback) {
                m_on_data_callback(std::string_view(m_read_buffer.data(), bytes_transferred));
            }
            DoRead();
        } else if (ec == boost::asio::error::eof) {
            spdlog::info("Ollama TCP socket closed cleanly by host (EOF).");
        } else {
            spdlog::error("TCP Socket read stream fault: {}", ec.message());
        }
    });
}

} // namespace malama::network
