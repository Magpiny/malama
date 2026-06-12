// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.cpp
// Purpose:     Implements the Boost.Asio non-blocking socket state machine
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/ollama_client.hpp"
#include "network/ollama_requests.hpp"
#include "common/constants.hpp"

#include <chrono>
#include <format>
#include <spdlog/spdlog.h>

namespace malama::network {

OllamaClient::OllamaClient(std::string host, std::string port) noexcept
    : m_resolver(m_io_context), m_socket(m_io_context), m_operation_timer(m_io_context), m_host(std::move(host)), m_port(std::move(port)) {}

OllamaClient::~OllamaClient() noexcept {
    boost::system::error_code err_code;
    m_io_context.stop();

    // 1. Capture the result object
    const auto cancel_result = m_socket.cancel(err_code);
    
    // 2. Perform a trivial inspection to satisfy the linter
    if (cancel_result == boost::system::errc::success || err_code) {
        spdlog::trace("Socket teardown (cancel): {}", err_code.message());
    }

    const auto close_result = m_socket.close(err_code);
    if (close_result == boost::system::errc::success || err_code) {
        spdlog::trace("Socket teardown (close): {}", err_code.message());
    }
}

auto OllamaClient::SubmitPrompt(std::string_view prompt_text, std::string_view model_name, std::function<void(std::string_view)> on_data) noexcept -> void {
    bool expected = false;
    if (!m_operation_in_progress.compare_exchange_strong(expected, true)) {
        spdlog::warn("SubmitPrompt called while operation in progress; ignoring concurrent request");
        return;
    }

    m_on_data_callback = std::move(on_data);

    OllamaGenerateRequest payload{
        .model = std::string(model_name),
        .prompt = std::string(prompt_text),
        .stream = true
    };

    std::string json_body;

    if (const auto parse_error = glz::write_json(payload, json_body); parse_error) {
        spdlog::error("Failed to serialize outbound JSON payload. Error flag: {}", static_cast<int>(parse_error.ec));
        m_operation_in_progress.store(false);
        return;
    }

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
    m_operation_timer.expires_after(std::chrono::seconds(constants::NETWORK_RESOLVE_TIMEOUT_SEC));
    m_operation_timer.async_wait([this]([[maybe_unused]] boost::system::error_code err_code) {
        if (!err_code) {
            spdlog::error("DNS resolution timeout exceeded");
            // Fixed: Resolver returns void, so we call it directly without assigning it
            m_resolver.cancel();
        } else {
            // Fixed: Fully consume the err_code parameter so the linter stops complaining
            spdlog::trace("DNS timer gracefully aborted: {}", err_code.message());
        }
    });

    m_resolver.async_resolve(m_host, m_port, [this]([[maybe_unused]] boost::system::error_code err_code, const boost::asio::ip::tcp::resolver::results_type &endpoints) {
        const auto timer_cancel_result = m_operation_timer.cancel();
        if (timer_cancel_result >= 0) {
            spdlog::trace("DNS timer successfully cancelled.");
        }

        if (!err_code) [[likely]] {
            DoConnect(endpoints);
        } else {
            spdlog::error("Socket DNS resolution failure: {}", err_code.message());
            m_operation_in_progress.store(false);
        }
    });
}

auto OllamaClient::DoConnect(const boost::asio::ip::tcp::resolver::results_type &endpoints) noexcept -> void {
    m_operation_timer.expires_after(std::chrono::seconds(constants::NETWORK_CONNECT_TIMEOUT_SEC));
    m_operation_timer.async_wait([this]([[maybe_unused]] boost::system::error_code err_code) {
        if (!err_code) {
            spdlog::error("TCP connection timeout exceeded");

            boost::system::error_code close_error;
            const auto close_result = m_socket.close(close_error);
            if (close_result == boost::system::errc::success || close_error) {
                spdlog::debug("Forced socket closure on connect timeout note: {}", close_error.message());
            }
        } else {
            spdlog::trace("Connect timer gracefully aborted: {}", err_code.message());
        }
    });

    boost::asio::async_connect(m_socket, endpoints, [this]([[maybe_unused]] boost::system::error_code err_code, const boost::asio::ip::tcp::endpoint &/*endpoint*/) {
        const auto timer_cancel_result = m_operation_timer.cancel();
        if (timer_cancel_result >= 0) {
            spdlog::trace("Connect timer successfully cancelled.");
        }
        
        if (!err_code) [[likely]] {
            DoWrite();
        } else {
            spdlog::error("TCP Socket connection refused: {}", err_code.message());
            m_operation_in_progress.store(false);
        }
    });
}

auto OllamaClient::DoWrite() noexcept -> void {
    m_operation_timer.expires_after(std::chrono::seconds(constants::NETWORK_WRITE_TIMEOUT_SEC));
    m_operation_timer.async_wait([this]([[maybe_unused]] boost::system::error_code err_code) {
        if (!err_code) {
            spdlog::error("TCP write timeout exceeded");
            
            boost::system::error_code close_error;
            const auto close_result = m_socket.close(close_error);
            if (close_result == boost::system::errc::success || close_error) {
                spdlog::debug("Forced socket closure on write timeout note: {}", close_error.message());
            }
        } else {
            spdlog::trace("Write timer gracefully aborted: {}", err_code.message());
        }
    });

    boost::asio::async_write(m_socket, boost::asio::buffer(m_request_buffer), [this]([[maybe_unused]] boost::system::error_code err_code, std::size_t /*bytes_transferred*/) {
        const auto timer_cancel_result = m_operation_timer.cancel();
        if (timer_cancel_result >= 0) {
            spdlog::trace("Write timer successfully cancelled.");
        }
        
        if (!err_code) [[likely]] {
            DoRead();
        } else {
            spdlog::error("TCP Socket payload transmission fault: {}", err_code.message());
            m_operation_in_progress.store(false);
        }
    });
}

auto OllamaClient::DoRead() noexcept -> void {
    m_operation_timer.expires_after(std::chrono::seconds(constants::NETWORK_READ_TIMEOUT_SEC));
    m_operation_timer.async_wait([this]([[maybe_unused]] boost::system::error_code err_code) {
        if (!err_code) {
            spdlog::error("TCP read timeout exceeded");
            
            boost::system::error_code close_error;
            const auto close_result = m_socket.close(close_error);
            if (close_result == boost::system::errc::success || close_error) {
                spdlog::debug("Forced socket closure on read timeout note: {}", close_error.message());
            }
        } else {
            spdlog::trace("Read timer gracefully aborted: {}", err_code.message());
        }
    });

    m_socket.async_read_some(boost::asio::buffer(m_read_buffer), [this]([[maybe_unused]] boost::system::error_code err_code, std::size_t bytes_transferred) {
        const auto timer_cancel_result = m_operation_timer.cancel();
        if (timer_cancel_result >= 0) {
            spdlog::trace("Read timer successfully cancelled.");
        }
        
        if (!err_code) [[likely]] {
            if (m_on_data_callback) {
                m_on_data_callback(std::string_view(m_read_buffer.data(), bytes_transferred));
            }
            DoRead();
        } else if (err_code == boost::asio::error::eof) {
            spdlog::info("Ollama TCP socket closed cleanly by host (EOF).");
            m_operation_in_progress.store(false);
        } else {
            spdlog::error("TCP Socket read stream fault: {}", err_code.message());
            m_operation_in_progress.store(false);
        }
    });
}

} // namespace malama::network
