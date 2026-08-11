/////////////////////////////////////////////////////////////////////////////
// Name:        tests/integration/test_system_e2e.cpp
// Purpose:     End-to-End integration test across Storage, Engine, and Exporter
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "engine/export/export_engine.hpp"
#include "engine/storage/history_manager.hpp"

namespace malama::tests {

TEST_CASE("v0.2.8-6: System-Level Integration (Storage -> Session -> Export)",
          "[integration][e2e]") {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "malama_e2e_test";
    std::filesystem::remove_all(test_dir);

    engine::storage::HistoryManager manager(test_dir);
    auto meta = manager.CreateSession("Integration Test Session");

    core::Message msg1{.m_id = "1",
                       .m_role = core::MessageRole::User,
                       .m_content = "Hello Engine",
                       .m_timestamp = 1000};
    core::Message msg2{.m_id = "2",
                       .m_role = core::MessageRole::Assistant,
                       .m_content = "Hello User",
                       .m_timestamp = 1001};

    manager.AppendMessage(meta.m_session_id, msg1);
    manager.AppendMessage(meta.m_session_id, msg2);

    auto loaded_session = manager.LoadSession(meta.m_session_id);
    REQUIRE(loaded_session.has_value());
    REQUIRE(loaded_session->m_messages.size() == 2);

    engine::export_sys::ExportEngine exporter;
    auto export_path = exporter.export_session(*loaded_session, test_dir,
                                               engine::export_sys::ExportFormat::Markdown);

    REQUIRE(export_path.has_value());
    REQUIRE(std::filesystem::exists(*export_path));

    std::filesystem::remove_all(test_dir);
}

}  // namespace malama::tests
