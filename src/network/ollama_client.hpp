// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.hpp
// Purpose:     Boost.Asio Native TCP HTTP Client Interface
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "common/constants.hpp"

#include <string>
#include <string_view>
#include <functional>
#include <array>
#include <thread>
#include <atomic>
#include <boost/asio.hpp>

namespace malama::network {

class OllamaClient final {
public:
    explicit OllamaClient(std::string host, std::string port) noexcept;
    ~OllamaClient() noexcept;

    OllamaClient(const OllamaClient &) = delete;
    OllamaClient &operator=(const OllamaClient &) = delete;
    OllamaClient(OllamaClient &&) noexcept = delete;
    OllamaClient &operator=(OllamaClient &&) noexcept = delete;

    auto SubmitPrompt(std::string_view prompt_text, std::string_view model_name, std::function<void(std::string_view)> on_data) noexcept -> void;

private:
    auto DoResolve() noexcept -> void;
    auto DoConnect(const boost::asio::ip::tcp::resolver::results_type &endpoints) noexcept -> void;
    auto DoWrite() noexcept -> void;
    auto DoRead() noexcept -> void;

    boost::asio::io_context m_io_context;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::steady_timer m_operation_timer;
    std::jthread m_context_thread;

    std::string m_host;
    std::string m_port;
    std::string m_request_buffer;
    std::array<char, constants::absolute_max_buffer_bytes> m_read_buffer{};
    std::function<void(std::string_view)> m_on_data_callback;
    std::atomic<bool> m_operation_in_progress{false};
};

} // namespace malama::network
