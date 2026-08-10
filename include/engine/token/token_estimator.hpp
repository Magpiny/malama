/////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/token/token_estimator.hpp
// Purpose:     Fast pre-flight context budget and attachment token estimator
// Author:      Wanjare S. (Magpiny)
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/models.hpp"

namespace malama::engine::token {

struct TokenBudgetStatus {
    uint32_t m_estimated_tokens{0};
    uint32_t m_context_limit{constants::max_chat_length};
    float m_usage_percentage{0.0F};
    bool m_is_overflow{false};
};

class TokenEstimator {
   public:
    [[nodiscard]] static uint32_t estimate_text_tokens(std::string_view text) noexcept;

    [[nodiscard]] static TokenBudgetStatus calculate_budget(
        std::string_view user_prompt, std::string_view system_prompt,
        const std::vector<core::Message> &history, uint32_t num_ctx,
        uint32_t attachment_token_overhead = 0) noexcept;
};

}  // namespace malama::engine::token
