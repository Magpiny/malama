// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_v030_subsystems.cpp
// Purpose:     Unit tests for v0.3.0 token estimation, export, and history subsystems
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <vector>

#include "core/models.hpp"
#include "engine/storage/attachment_manager.hpp"
#include "engine/token/token_estimator.hpp"

namespace malama::tests {

TEST_CASE("TokenEstimator calculates text and payload tokens accurately", "[unit][token][budget]") {
    const engine::token::TokenEstimator estimator;
    const std::vector<engine::storage::AttachmentInfo> pending_attachments;
    const std::vector<core::Message> history;

    SECTION("Estimates raw text token counts based on heuristic ratio") {
        // 12 characters should estimate to ~3 tokens (4 chars per token ratio)
        const std::string text = "123456789012";
        const std::size_t estimated_tokens = estimator.estimate_text_tokens(text);

        REQUIRE(estimated_tokens == 3UZ);
    }

    SECTION("Flags context limit overflow when payload exceeds limit") {
        // 3000 characters ≈ 750 tokens
        const std::string massive_prompt(3000, 'x');
        const std::size_t num_ctx_limit = 500UZ;

        const std::size_t estimated_tokens =
            estimator.estimate_payload_tokens(massive_prompt, pending_attachments, history);

        const bool is_overflow = estimated_tokens >= num_ctx_limit;

        REQUIRE(estimated_tokens >= num_ctx_limit);
        REQUIRE(is_overflow == true);
    }

    SECTION("Normal prompt payload fits safely within context budget") {
        const std::string short_prompt = "Hello Ollama";
        const std::size_t num_ctx_limit = 2048UZ;

        const std::size_t estimated_tokens =
            estimator.estimate_payload_tokens(short_prompt, pending_attachments, history);

        const bool is_overflow = estimated_tokens >= num_ctx_limit;
        const float usage_percentage =
            (static_cast<float>(estimated_tokens) / static_cast<float>(num_ctx_limit)) * 100.0F;

        REQUIRE(is_overflow == false);
        REQUIRE(usage_percentage < 10.0F);
    }
}

}  // namespace malama::tests
