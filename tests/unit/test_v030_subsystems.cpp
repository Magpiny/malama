/////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_v030_subsystems.cpp
// Purpose:     Unit tests for v0.3.0 token estimation, export, and history subsystems
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "engine/token/token_estimator.hpp"

namespace malama::tests {

TEST_CASE("TokenEstimator calculates text context budget accurately", "[unit][token][budget]") {
    SECTION("Flags overflow when context limit is exceeded") {
        // 3000 characters ≈ 1000 tokens
        std::string massive_prompt(3000, 'x');
        std::vector<core::Message> history;

        // Set context limit to 500 tokens
        auto budget =
            engine::token::TokenEstimator::calculate_budget(massive_prompt, "", history, 500);

        REQUIRE(budget.m_estimated_tokens >= 500);
        REQUIRE(budget.m_is_overflow == true);
    }

    SECTION("Normal context fits within limit") {
        std::string short_prompt = "Hello Ollama";
        std::vector<core::Message> history;

        auto budget =
            engine::token::TokenEstimator::calculate_budget(short_prompt, "", history, 2048);

        REQUIRE(budget.m_is_overflow == false);
        REQUIRE(budget.m_usage_percentage < 10.0f);
    }
}

}  // namespace malama::tests
