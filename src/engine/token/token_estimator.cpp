/////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/token/token_estimator.cpp
// Purpose:     Pre-flight token estimation algorithms for context budgeting
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/token/token_estimator.hpp"

#include <cmath>

namespace malama::engine::token {

[[nodiscard]] uint32_t TokenEstimator::estimate_text_tokens(std::string_view text) noexcept {
    if (text.empty()) {
        return 0;
    }
    // Heuristic estimation (~3.8 character count per token for text/code)
    return static_cast<uint32_t>(std::ceil(static_cast<float>(text.size()) / 3.8F));
}

[[nodiscard]] TokenBudgetStatus TokenEstimator::calculate_budget(
    std::string_view user_prompt, std::string_view system_prompt,
    const std::vector<core::Message> &history, uint32_t num_ctx,
    uint32_t attachment_token_overhead) noexcept {
    uint32_t total_tokens = estimate_text_tokens(user_prompt) +
                            estimate_text_tokens(system_prompt) + attachment_token_overhead;

    for (const auto &msg : history) {
        total_tokens += estimate_text_tokens(msg.m_content);
    }

    TokenBudgetStatus status;
    status.m_estimated_tokens = total_tokens;
    status.m_context_limit = (num_ctx > 0) ? num_ctx : constants::max_chat_length;
    status.m_usage_percentage =
        (static_cast<float>(total_tokens) / static_cast<float>(status.m_context_limit)) * 100.0F;
    status.m_is_overflow = total_tokens >= status.m_context_limit;

    return status;
}

}  // namespace malama::engine::token
