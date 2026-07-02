// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.hpp
// Purpose:     Asynchronous stream coordinator handling chunk fragmentation
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-11
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: Apache-2.0

#include "network/ollama_client.hpp"
#include "core/models.hpp"

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
    auto operator=(const StreamWorker &) -> StreamWorker & = delete;
    StreamWorker(StreamWorker &&) noexcept = default;
    auto operator=(StreamWorker &&) noexcept -> StreamWorker & = default;

    auto InitializeGeneration(
        std::string_view model_name,
        std::string_view prompt_text,
        const std::vector<core::Message> &history_context,
        std::function<void(std::string_view, bool)> token_callback
    ) noexcept -> void;

    auto IngestRawNetworkBytes(std::string_view incoming_bytes) noexcept -> void;

private:
    std::unique_ptr<OllamaClient> m_client_ptr;
    std::function<void(std::string_view, bool)> m_token_callback;
    std::string m_residual_buffer{};
};

} // namespace malama::network
