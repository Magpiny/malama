// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_v030_subsystems.cpp
// Purpose:     Unit tests for v0.3.0 token estimation, export, and history subsystems
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "core/models.hpp"
#include "engine/storage/attachment_manager.hpp"
#include "engine/storage/history_manager.hpp"
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

TEST_CASE("HistoryManager handles custom SessionParameters properly", "[unit][storage][params]") {
    std::filesystem::path test_dir =
        std::filesystem::temp_directory_path() / "malama_params_test";
    std::filesystem::remove_all(test_dir);

    engine::storage::HistoryManager manager(test_dir);

    core::ModelParameters custom_params{
        .m_temperature = 1.35F,
        .m_top_p = 0.85F,
        .m_top_k = 75,
        .m_repeat_penalty = 1.25F,
        .m_num_ctx = 32768,
        .m_system_prompt = "You are an expert C++ assistant."
    };

    auto meta = manager.CreateSession("Params Test Session", custom_params);
    auto loaded = manager.LoadSession(meta.m_session_id);

    REQUIRE(loaded.has_value());
    REQUIRE(loaded->m_metadata.m_parameters.m_temperature == Catch::Approx(1.35F));
    REQUIRE(loaded->m_metadata.m_parameters.m_top_p == Catch::Approx(0.85F));
    REQUIRE(loaded->m_metadata.m_parameters.m_top_k == 75);
    REQUIRE(loaded->m_metadata.m_parameters.m_repeat_penalty == Catch::Approx(1.25F));
    REQUIRE(loaded->m_metadata.m_parameters.m_num_ctx == 32768);
    REQUIRE(loaded->m_metadata.m_parameters.m_system_prompt == "You are an expert C++ assistant.");

    // Test parameter mutation update
    custom_params.m_temperature = 0.2F;
    custom_params.m_num_ctx = 65536;
    manager.UpdateSessionParameters(meta.m_session_id, custom_params);

    auto updated = manager.LoadSession(meta.m_session_id);
    REQUIRE(updated.has_value());
    REQUIRE(updated->m_metadata.m_parameters.m_temperature == Catch::Approx(0.2F));
    REQUIRE(updated->m_metadata.m_parameters.m_num_ctx == 65536);

    std::filesystem::remove_all(test_dir);
}

}  // namespace malama::tests
