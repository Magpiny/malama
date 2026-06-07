// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.hpp
// Purpose:     Coordinates non-blocking parallel execution threads via jthread
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-06
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "network/i_ollama_client.hpp"
#include <memory>
#include <thread>

namespace malama::network {

class StreamWorker final {
public:
    explicit StreamWorker(std::unique_ptr<IOllamaClient> target_client) noexcept;
    ~StreamWorker();

    StreamWorker(const StreamWorker &) = delete;
    StreamWorker &operator=(const StreamWorker &) = delete;
    StreamWorker(StreamWorker &&) noexcept = default;
    StreamWorker &operator=(StreamWorker &&) noexcept = default;

    auto InitializeGeneration(
        std::string model_name,
        std::vector<common::Message> historical_chain,
        std::move_only_function<void(std::string_view)> on_token_received
    ) noexcept -> void;

    auto TerminateGeneration() noexcept -> void;

    [[nodiscard]] auto IsGenerationActive() const noexcept -> bool;

private:
    std::unique_ptr<IOllamaClient> m_network_client;
    std::jthread m_background_thread;
};

} // namespace malama::network
