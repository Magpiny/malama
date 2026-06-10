// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.hpp
// Purpose:     Asynchronous stream coordinator handling chunk fragmentation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "network/ollama_client.hpp"
#include "common/types.hpp" // Fixed: Pointed cleanly to our true types mapping file

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>

namespace malama::network {

class StreamWorker final {
public:
    explicit StreamWorker(std::unique_ptr<OllamaClient> client_ptr) noexcept;
    ~StreamWorker() = default;

    StreamWorker(const StreamWorker &) = delete;
    StreamWorker &operator=(const StreamWorker &) = delete;
    StreamWorker(StreamWorker &&) noexcept = default;
    StreamWorker &operator=(StreamWorker &&) noexcept = default;

    auto InitializeGeneration(
        std::string_view model_name,
        const std::vector<common::Message> &history_context,
        std::function<void(std::string_view)> token_callback
    ) noexcept -> void;

    auto IngestRawNetworkBytes(std::string_view incoming_bytes) noexcept -> void;

private:
    std::unique_ptr<OllamaClient> m_client_ptr;
    std::function<void(std::string_view)> m_token_callback;
    std::string m_residual_buffer{};
};

} // namespace malama::network
