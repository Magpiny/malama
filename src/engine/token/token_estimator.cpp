// /////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/token/token_estimator.cpp
// Purpose:     Implements token estimation heuristic calculations
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-12
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/token/token_estimator.hpp"

#include <algorithm>
#include <cstddef>

namespace malama::engine::token {

inline constexpr std::size_t chars_per_token_ratio = 4UZ;
inline constexpr std::size_t message_overhead_tokens = 4UZ;  // e.g. <|im_start|>role...<|im_end|>

[[nodiscard]] auto TokenEstimator::estimate_text_tokens(std::string_view text) const noexcept
    -> std::size_t {
    if (text.empty()) {
        return 0UZ;
    }
    return std::max(1UZ, text.size() / chars_per_token_ratio);
}

[[nodiscard]] auto TokenEstimator::estimate_payload_tokens(
    std::string_view current_prompt,
    const std::vector<storage::AttachmentInfo> &pending_attachments,
    const std::vector<core::Message> &history) const noexcept -> std::size_t {
    std::size_t total_tokens = 0UZ;

    // 1. Context history tokens
    for (const auto &message : history) {
        total_tokens += estimate_text_tokens(message.m_content) + message_overhead_tokens;
    }

    // 2. Pending text attachment tokens
    for (const auto &attachment : pending_attachments) {
        if (attachment.m_type == storage::AttachmentType::TEXT_DOCUMENT) {
            auto content = storage::AttachmentManager::ExtractTextContent(attachment);
            if (content.has_value()) {
                total_tokens += estimate_text_tokens(content.value());
            }
        }
    }

    // 3. Current active prompt input tokens
    total_tokens += estimate_text_tokens(current_prompt);

    return total_tokens;
}

}  // namespace malama::engine::token
