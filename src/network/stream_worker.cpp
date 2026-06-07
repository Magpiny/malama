// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.cpp
// Purpose:     Thread execution logic and cooperative structural cancellation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-07
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/stream_worker.hpp"
#include <spdlog/spdlog.h>

namespace malama::network {

StreamWorker::StreamWorker(std::unique_ptr<IOllamaClient> target_client) noexcept
    : m_network_client(std::move(target_client)) {}

StreamWorker::~StreamWorker() {
    TerminateGeneration();
}

auto StreamWorker::InitializeGeneration(
    std::string model_name,
    std::vector<common::Message> historical_chain,
    std::move_only_function<void(std::string_view)> on_token_received
) noexcept -> void {
    TerminateGeneration();

    spdlog::info("Spawning background text processing execution context...");

    m_background_thread = std::jthread([
        this,
        model = std::move(model_name),
        history = std::move(historical_chain),
        callback = std::move(on_token_received)
    ](std::stop_token stop) mutable {
        // Resolved: Added std::move() wrapper to prevent token parameter copying
        auto result = m_network_client->RequestStream(
            model, history, std::move(callback), std::move(stop)
        );
        if (!result) {
            spdlog::warn("Background ingestion stream closed out early.");
        }
    });
}

auto StreamWorker::TerminateGeneration() noexcept -> void {
    if (m_background_thread.joinable()) {
        spdlog::debug("Sending cooperative cancellation request signal...");
        m_background_thread.request_stop();
        m_background_thread.join();
    }
}

bool StreamWorker::IsGenerationActive() const noexcept {
    return m_background_thread.joinable();
}

} // namespace malama::network
