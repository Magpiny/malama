/////////////////////////////////////////////////////////////////////////////
// Name:        include/engine/export/export_engine.hpp
// Purpose:     Multi-format thread exporter (Markdown, JSON, PlainText)
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-10
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#pragma once

// SPDX-License-Identifier: GPL-3.0-or-later

#include <expected>
#include <filesystem>
#include <string>

#include "core/models.hpp"

namespace malama::engine::export_sys {

enum class ExportFormat : uint8_t { Markdown, Json, PlainText };

enum class ExportError : uint8_t {
    FileAccessDenied,
    InvalidDirectory,
    EmptySession,
    SerializationFailed
};

class ExportEngine {
   public:
    [[nodiscard]] std::expected<std::filesystem::path, ExportError> export_session(
        const core::ChatSession &session, const std::filesystem::path &target_dir,
        ExportFormat format = ExportFormat::Markdown) const;
};

}  // namespace malama::engine::export_sys
