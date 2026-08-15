// /////////////////////////////////////////////////////////////////////////////
// Name:        include/network/ollama_client.hpp
// Purpose:     Asynchronous Cobalt-driven HTTP communication controller
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-07-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/cobalt/task.hpp>
#include <expected>
#include <functional>
#include <glaze/glaze.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/types.hpp"
#include "core/models.hpp"

namespace malama::network {

/**
 * @brief Nested message object inside each streamed /api/chat response line.
 *        Ollama's chat endpoint emits token text here, not at a top-level
 *        "response" key (that shape belongs to /api/generate, not /api/chat).
 */
struct ResponseMessage final {
    std::string m_role{};
    std::string m_content{};
    struct glaze {
        using Type = ResponseMessage;
        static constexpr auto value =
            glz::object("role", &Type::m_role, "content", &Type::m_content);
    };
};

/**
 * @brief Inbound response frame chunk received from the Ollama /api/chat socket stream.
 *        Real shape:
 * {"model":"...","created_at":"...","message":{"role":"assistant","content":"..."},"done":false}
 */
struct ResponseChunk final {
    ResponseMessage m_message{};
    bool m_done{false};
    struct glaze {
        using Type = ResponseChunk;
        static constexpr auto value =
            glz::object("message", &Type::m_message, "done", &Type::m_done);
    };
};

/**
 * @brief Asynchronous HTTP network client driving local Ollama socket streaming operations.
 */
class OllamaClient final {
   public:
    explicit OllamaClient(std::string host_name, std::string port_number) noexcept;
    ~OllamaClient() noexcept;

    OllamaClient(const OllamaClient &) = delete;
    auto operator=(const OllamaClient &) -> OllamaClient & = delete;
    OllamaClient(OllamaClient &&) noexcept = delete;
    auto operator=(OllamaClient &&) noexcept -> OllamaClient & = delete;

    [[nodiscard]] auto GetExecutor() noexcept -> boost::asio::io_context::executor_type;

    auto ExecuteStreamTask(std::string model_name, const std::vector<core::Message> &history,
                           std::string prompt_text, std::vector<std::string> images_payload,
                           std::function<void(std::string_view)> token_callback) noexcept
        -> boost::cobalt::task<std::expected<void, common::NetworkError>>;

    auto ExecuteStreamTask(std::string model_name, const std::vector<core::Message> &history,
                           std::string prompt_text, common::SessionParameters params,
                           std::vector<std::string> images_payload,
                           std::function<void(std::string_view)> token_callback) noexcept
        -> boost::cobalt::task<std::expected<void, common::NetworkError>>;

    void TriggerActiveGenerationCancellation() noexcept;

   private:
    struct ResponseCache final {
        static constexpr std::size_t max_cache_entries = 64UZ;
        std::unordered_map<std::string, std::string> m_lookup_table{};
        std::vector<std::string> m_access_order{};
    };

    auto CheckCache(const std::string &cache_key) noexcept -> std::optional<std::string>;

    void UpdateCache(const std::string &cache_key, const std::string &response_value) noexcept;

    // THIS IS THE REAL DEAL!! HERE
    [[nodiscard]] auto PrepareHttpRequest(std::string model_name,
                                          const std::vector<core::Message> &history,
                                          std::string prompt_text,
                                          const common::SessionParameters &params,
                                          std::vector<std::string> images_payload) const noexcept
        -> boost::beast::http::request<boost::beast::http::string_body>;

    void ParseStreamAccumulator(const std::function<void(const std::string_view)> &token_callback,
                                std::string &complete_stream_accumulation) noexcept;

    boost::asio::io_context m_io_context{};
    boost::asio::ip::tcp::socket m_socket;
    std::string m_host{};
    std::string m_port{};

    std::atomic<bool> m_is_streaming{false};
    std::atomic<bool> m_cancellation_requested{false};
    std::string m_residual_line_accumulator{};

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;
    std::thread m_worker_thread{};
    ResponseCache m_cache_store{};
};

}  // namespace malama::network
