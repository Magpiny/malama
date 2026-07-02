// /////////////////////////////////////////////////////////////////////////////
// Name:        src/network/stream_worker.cpp
// Purpose:     Implements line-splitting algorithms and non-throwing JSON decoding
// Author:      Wanjare S.<samuelwanjare@protonmail.com>
// Created:     2026-06-11
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
    std::string_view prompt_text,
    [[maybe_unused]] const std::vector<core::Message> &history_context,
    std::function<void(std::string_view, bool)> token_callback
) noexcept -> void {
    m_token_callback = std::move(token_callback);
    m_residual_buffer.clear();

    spdlog::debug("Stream worker target initialized for active model: {}", model_name);

    m_client_ptr->SubmitPrompt(prompt_text, model_name, [this](std::string_view raw_chunk) {
        IngestRawNetworkBytes(raw_chunk);
    });
}

auto StreamWorker::IngestRawNetworkBytes(std::string_view incoming_bytes) noexcept -> void {
    if (!m_token_callback) [[unlikely]] {
        return;
    }

    try {
        const std::size_t allowed = constants::absolute_max_buffer_bytes - m_residual_buffer.size();
        if (allowed == 0) {
            spdlog::warn("Residual buffer limit reached; flushing allocation frame context.");
            m_residual_buffer.clear();
            return;
        }

        const std::size_t bytes_to_append = std::min(incoming_bytes.size(), allowed);
        m_residual_buffer.append(incoming_bytes.data(), bytes_to_append);
    } catch (const std::bad_alloc &) {
        spdlog::error("Memory allocation failure in IngestRawNetworkBytes; clearing buffer");
        m_residual_buffer.clear();
        return;
    }

    std::string_view processing_view(m_residual_buffer);
    std::size_t newline_position = std::string_view::npos;

    while ((newline_position = processing_view.find('\n')) != std::string_view::npos) {
        std::string_view current_line = processing_view.substr(0, newline_position);
        processing_view.remove_prefix(newline_position + 1);
        
        if (!current_line.empty() && current_line.back() == '\r') {
            current_line.remove_suffix(1);
        }

        if (!current_line.empty() && current_line.front() == '{') {
            OllamaGenerateChunk parsed_chunk{};
            
            const auto execution_error = glz::read<glz::opts{.error_on_unknown_keys = false}>(
                parsed_chunk, current_line
            );
            
            if (!execution_error) [[likely]] {
                if (!parsed_chunk.response.empty()) {
                    // Send parsed text token segment down the async bridge pipeline
                    m_token_callback(parsed_chunk.response, false);
                }
                if (parsed_chunk.done) {
                    spdlog::info("Ollama discrete token generation stream marked finalized.");
                    // Fire termination marker pass containing validation flag state
                    m_token_callback("", true);
                }
            } else {
                spdlog::warn(
                    "Glaze parser discarded corrupted frame segment. Error code: {}", 
                    static_cast<int>(execution_error.ec)
                );
            }
        }
    }

    m_residual_buffer = std::string(processing_view);
}

} // namespace malama::network
