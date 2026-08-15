// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.cpp
// Purpose:     Drives Cobalt task execution loops cleanly with zero threads leaked
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////
#include "network/stream_worker.hpp"

#include <boost/asio/detached.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/cobalt/task.hpp>
#include <spdlog/spdlog.h>

#include "config/config_manager.hpp"
#include "network/base64.hpp"
namespace malama::network {
inline constexpr std::size_t think_open_tag_length = 7UZ;
inline constexpr std::size_t think_close_tag_length = 8UZ;
StreamWorker::StreamWorker(std::unique_ptr<OllamaClient> client_ptr) noexcept
    : m_client_ptr(std::move(client_ptr)) {}
auto RunCobaltTask(OllamaClient *client_ptr, std::string model_name,
                   std::vector<core::Message> history, std::string prompt_text,
                   std::vector<std::string> images_payload, bool thinking_enabled,
                   std::function<void(std::string_view, bool)> token_callback)
    -> boost::cobalt::task<void> {
    bool is_inside_thinking_block = false;
    static_cast<void>(co_await client_ptr->ExecuteStreamTask(
        model_name, history, prompt_text, images_payload,
        [token_callback, thinking_enabled, &is_inside_thinking_block](std::string_view token) {
            if (!token_callback) {
                return;
            }
            if (thinking_enabled) {
                token_callback(token, false);
                return;
            }
            std::string token_buffer(token);
            if (token_buffer.contains("<think>")) {
                is_inside_thinking_block = true;
                auto position = token_buffer.find("<think>");
                std::string before_block = token_buffer.substr(0, position);
                if (!before_block.empty()) {
                    token_callback(before_block, false);
                }
                token_buffer = token_buffer.substr(position + think_open_tag_length);
            }
            if (is_inside_thinking_block) {
                if (token_buffer.contains("</think>")) {
                    is_inside_thinking_block = false;
                    auto position = token_buffer.find("</think>");
                    std::string after_block =
                        token_buffer.substr(position + think_close_tag_length);
                    if (!after_block.empty()) {
                        token_callback(after_block, false);
                    }
                }
            } else {
                if (!token_buffer.empty()) {
                    token_callback(token_buffer, false);
                }
            }
        }));
    if (token_callback) {
        token_callback("", true);
    }
    co_return;
}
auto StreamWorker::InitializeGeneration(
    std::string_view model_name, std::string_view prompt_text,
    const std::vector<core::Message> &history_context,
    std::function<void(std::string_view, bool)> token_callback) noexcept -> void {
    m_token_callback = std::move(token_callback);
    spdlog::debug("Cobalt workspace stream task initialized for reasoning validation pipelines.");
    const auto app_config = config::ConfigManager::get_instance().get_config();
    const bool thinking_enabled = app_config.m_engine.m_thinking_enabled;
    // Collect the staged image strings populated by the chat interface thread
    auto images_payload = malama::network::ImageTransit::MovePendingImages();
    if (m_client_ptr) {
        boost::cobalt::spawn(
            m_client_ptr->GetExecutor(),
            RunCobaltTask(m_client_ptr.get(), std::string(model_name),
                          std::vector<core::Message>(history_context), std::string(prompt_text),
                          std::move(images_payload), thinking_enabled, m_token_callback),
            boost::asio::detached);
    }
}
}  // namespace malama::network
