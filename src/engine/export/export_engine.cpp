/////////////////////////////////////////////////////////////////////////////
// Name:        src/engine/export/export_engine.cpp
// Purpose:     Export engine implementation for conversation persistence
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/export/export_engine.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <glaze/glaze.hpp>

namespace malama::engine::export_sys {

[[nodiscard]] std::expected<std::filesystem::path, ExportError> ExportEngine::export_session(
    const core::ChatSession &session, const std::filesystem::path &target_dir,
    ExportFormat format) const {
    if (session.m_messages.empty()) {
        return std::unexpected(ExportError::EmptySession);
    }

    std::error_code err;
    if (!std::filesystem::exists(target_dir) &&
        !std::filesystem::create_directories(target_dir, err)) {
        return std::unexpected(ExportError::InvalidDirectory);
    }

    std::string ext = (format == ExportFormat::Markdown) ? ".md"
                      : (format == ExportFormat::Json)   ? ".json"
                                                         : ".txt";

    std::string safe_title = session.m_metadata.m_title;
    std::ranges::replace_if(
        safe_title, [](char xtr) { return !std::isalnum(xtr) && xtr != '_'; }, '_');

    std::filesystem::path file_path =
        target_dir / std::format("{}_{}{}", safe_title, session.m_metadata.m_updated_at, ext);
    std::ofstream out_file(file_path, std::ios::out | std::ios::trunc);

    if (!out_file.is_open()) {
        return std::unexpected(ExportError::FileAccessDenied);
    }

    if (format == ExportFormat::Markdown) {
        out_file << std::format("# Chat Session: {}\n\n", session.m_metadata.m_title);
        out_file << std::format("*Created Epoch: {} | Context Window: {}*\n\n---\n\n",
                                session.m_metadata.m_created_at,
                                session.m_metadata.m_parameters.m_num_ctx);

        for (const auto &msg : session.m_messages) {
            std::string role_str = (msg.m_role == core::MessageRole::User)        ? "User"
                                   : (msg.m_role == core::MessageRole::Assistant) ? "Assistant"
                                                                                  : "System";
            out_file << std::format("### **{}**\n{}\n\n---\n\n", role_str, msg.m_content);
        }
    } else if (format == ExportFormat::PlainText) {
        for (const auto &msg : session.m_messages) {
            std::string role_str = (msg.m_role == core::MessageRole::User)        ? "USER"
                                   : (msg.m_role == core::MessageRole::Assistant) ? "ASSISTANT"
                                                                                  : "SYSTEM";
            out_file << std::format("[{}]\n{}\n\n", role_str, msg.m_content);
        }
    } else if (format == ExportFormat::Json) {
        std::string json_buffer{};
        if (auto err_c = glz::write_json(session, json_buffer); err_c) {
            return std::unexpected(ExportError::SerializationFailed);
        }
        out_file << json_buffer;
    }

    return file_path;
}

}  // namespace malama::engine::export_sys
