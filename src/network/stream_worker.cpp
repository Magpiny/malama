// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.cpp
// Purpose:     Implements line-splitting algorithms and non-throwing JSON decoding
// Author:      Wanjare <wanjare@magpiny.dev>
// Created:     2026-06-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     Apache-2.0
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: Apache-2.0

#include "network/stream_worker.hpp"
#include "network/ollama_chunks.hpp"

#include <spdlog/spdlog.h>

namespace malama::network {

StreamWorker::StreamWorker(std::unique_ptr<OllamaClient> client_ptr) noexcept
    : m_client_ptr(std::move(client_ptr)) {}

auto StreamWorker::InitializeGeneration(
    std::string_view model_name,
    [[maybe_unused]] const std::vector<common::Message> &history_context,
    std::function<void(std::string_view)> token_callback
) noexcept -> void {
    m_token_callback = std::move(token_callback);
    m_residual_buffer.clear();

    spdlog::debug("Stream worker target initialized for active model: {}", model_name);
}

auto StreamWorker::IngestRawNetworkBytes(std::string_view incoming_bytes) noexcept -> void {
    if (!m_token_callback) [[unlikely]] {
        return;
    }

    // Accumulate incoming data packets into our internal residual buffer
    m_residual_buffer.append(incoming_bytes.data(), incoming_bytes.size());

    std::string_view processing_view(m_residual_buffer);
    std::size_t newline_position = std::string_view::npos;

    // Process all fully bounded lines found inside our window view
    while ((newline_position = processing_view.find('\n')) != std::string_view::npos) {
        std::string_view current_line = processing_view.substr(0, newline_position);
        
        if (!current_line.empty()) {
            OllamaGenerateChunk parsed_chunk{};
            
            // Modern C++ exception-free JSON extraction using Glaze
            const auto execution_error = glz::read_json(parsed_chunk, current_line);
            
            if (!execution_error) [[likely]] {
                if (!parsed_chunk.response.empty()) {
                    m_token_callback(parsed_chunk.response);
                }
                if (parsed_chunk.done) {
                    spdlog::info("Ollama discrete token generation stream marked finalized.");
                }
            } else {
                // Safely log and skip any truncated or malformed frames without crashing the thread
                spdlog::warn("Glaze parser discarded corrupted frame segment. Details: {}", static_cast<int>(execution_error.ec));
            }
        }
        
        // Advance the processing window past the newline marker
        processing_view.remove_prefix(newline_position + 1);
    }

    // Keep any remaining unfinalized string data in the residual buffer for the next network packet
    m_residual_buffer = std::string(processing_view);
}

} // namespace malama::network
