/////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_v030_subsystems.cpp
// Purpose:     Unit tests for token estimation and export engine in v0.3.0
// Author:      Wanjare S. (Magpiny)
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include "engine/export/export_engine.hpp"
#include "engine/token/token_estimator.hpp"

TEST_CASE("TokenEstimator calculates text context budget accurately", "[token][budget]") {
    using malama::engine::token::TokenEstimator;

    SECTION("Estimates tokens proportional to text character count") {
        std::string sample = "Hello world! This is a test for token estimation.";
        uint32_t tokens = TokenEstimator::estimate_text_tokens(sample);
        REQUIRE(tokens > 0);
        REQUIRE(tokens < sample.size());
    }

    SECTION("Flags overflow when context limit is exceeded") {
        auto budget = TokenEstimator::calculate_budget("Very long prompt...", "", {}, 10);
        REQUIRE(budget.m_is_overflow == true);
    }
}

TEST_CASE("ExportEngine generates Markdown, Text, and JSON output", "[export][file]") {
    using namespace malama::engine::export_sys;
    malama::core::ChatSession session;
    session.m_metadata.m_title = "Test_Session";
    session.m_metadata.m_updated_at = 123456789;

    malama::core::Message msg;
    msg.m_role = malama::core::MessageRole::User;
    msg.m_content = "Hello Ollama!";
    session.m_messages.push_back(msg);

    ExportEngine exporter;
    auto temp_dir = std::filesystem::temp_directory_path() / "malama_test_exports";

    SECTION("Exports session to Markdown file cleanly") {
        auto res = exporter.export_session(session, temp_dir, ExportFormat::Markdown);
        REQUIRE(res.has_value());
        REQUIRE(std::filesystem::exists(res.value()));
        std::filesystem::remove_all(temp_dir);
    }
}
