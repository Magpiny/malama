// /////////////////////////////////////////////////////////////////////////////
// Name:        include/network/ollama_client.hpp
// Purpose:     Asynchronous Cobalt-driven HTTP communication controller
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/cobalt/task.hpp>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/types.hpp"

namespace malama::network {

class OllamaClient final {
   public:
    explicit OllamaClient(std::string host_name, std::string port_number) noexcept;
    ~OllamaClient() noexcept;

    OllamaClient(const OllamaClient &) = delete;
    auto operator=(const OllamaClient &) -> OllamaClient & = delete;
    OllamaClient(OllamaClient &&) noexcept = delete;
    auto operator=(OllamaClient &&) noexcept -> OllamaClient & = delete;

    /// @brief Provides access to the dedicated background thread executor context
    [[nodiscard]] auto GetExecutor() noexcept -> boost::asio::io_context::executor_type;

    /// @brief Executes a suspended stackless Cobalt task handling multimodal streams.
    auto ExecuteStreamTask(std::string_view model_name, std::string_view prompt_text,
                           const std::vector<std::string> &images_payload,
                           std::function<void(std::string_view)> token_callback) noexcept
        -> boost::cobalt::task<std::expected<void, common::NetworkError>>;

   private:
    struct ResponseCache final {
        static constexpr std::size_t max_cache_entries = 64uz;
        std::unordered_map<std::string, std::string> m_lookup_table;
        std::vector<std::string> m_access_order;
    };

    auto CheckCache(const std::string &cache_key) noexcept -> std::optional<std::string>;
    auto UpdateCache(const std::string &cache_key, const std::string &response_value) noexcept
        -> void;

    boost::asio::io_context m_io_context;
    boost::asio::ip::tcp::socket m_socket;
    std::string m_host;
    std::string m_port;
    std::atomic<bool> m_is_streaming{false};

    // Background orchestration pipeline layer targets
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;
    std::jthread m_worker_thread;
    ResponseCache m_cache_store;
};

}  // namespace malama::network
