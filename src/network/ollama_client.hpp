// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/ollama_client.hpp
// Purpose:     Asynchronous coroutine-driven HTTP communication controller
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <atomic>
#include <boost/asio.hpp>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "common/types.hpp"

namespace malama::network {

class OllamaClient final {
   public:
    explicit OllamaClient(std::string host, std::string port) noexcept;
    ~OllamaClient() noexcept;

    OllamaClient(const OllamaClient &) = delete;
    auto operator=(const OllamaClient &) -> OllamaClient & = delete;
    OllamaClient(OllamaClient &&) noexcept = delete;
    auto operator=(OllamaClient &&) noexcept -> OllamaClient & = delete;

    /// @brief Executes a suspended stackless coroutine handling multimodal streaming.
    /// @param model Target model descriptor profile.
    /// @param prompt Text input or context injected document block.
    /// @param image_base64_payload Optional vector containing Base64 formatted visual assets.
    /// @param on_token Handler triggered asynchronously as stream segments land.
    auto ExecuteStreamTask(std::string_view model, std::string_view prompt,
                           const std::vector<std::string> &image_base64_payload,
                           std::function<void(std::string_view)> on_token) noexcept
        -> boost::asio::awaitable<std::expected<void, common::NetworkError>>;

   private:
    boost::asio::io_context m_io_context;
    boost::asio::ip::tcp::socket m_socket;
    std::string m_host;
    std::string m_port;
    std::atomic<bool> m_is_streaming{false};
};

}  // namespace malama::network
