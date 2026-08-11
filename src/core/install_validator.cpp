/////////////////////////////////////////////////////////////////////////////
// Name:        src/core/install_validator.cpp
// Purpose:     Validation routines for system installation and cleanup
// Author:      Wanjare S. (Magpiny)
// Created:     2026-08-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#include "core/install_validator.hpp"

#include <cstdlib>
#include <fstream>
#include <spdlog/spdlog.h>

namespace malama::core {

InstallValidator::InstallValidator(std::filesystem::path home_override) {
    if (!home_override.empty()) {
        m_home_path = std::move(home_override);
    } else {
        const char *env_home = std::getenv("HOME");
        m_home_path = (env_home != nullptr) ? env_home : "";
    }

    m_bin_path = m_home_path / ".local" / "bin" / "malama";
    m_config_path = m_home_path / ".config" / "malama";
    m_desktop_path = m_home_path / ".local" / "share" / "applications" / "malama.desktop";
}

[[nodiscard]] std::expected<void, ValidationError> InstallValidator::validate_environment() const {
    if (m_home_path.empty() || !std::filesystem::exists(m_home_path)) {
        spdlog::error("Invalid HOME environment path.");
        return std::unexpected(ValidationError::HomeDirNotFound);
    }

    std::error_code err;
    std::filesystem::create_directories(m_config_path, err);
    if (err) {
        spdlog::error("Failed to ensure config directory: {}", err.message());
        return std::unexpected(ValidationError::ConfigDirectoryUnwritable);
    }

    return {};
}

[[nodiscard]] std::expected<void, ValidationError> InstallValidator::verify_desktop_integration()
    const {
    if (!std::filesystem::exists(m_bin_path)) {
        spdlog::warn("Executable binary missing at expected location: {}", m_bin_path.string());
        return std::unexpected(ValidationError::BinaryNotInPath);
    }

    if (!std::filesystem::exists(m_desktop_path)) {
        spdlog::warn("XDG Desktop shortcut missing at: {}", m_desktop_path.string());
        return std::unexpected(ValidationError::DesktopEntryMissing);
    }

    return {};
}

[[nodiscard]] bool InstallValidator::check_orphan_cleanup() const {
    const bool bin_exists = std::filesystem::exists(m_bin_path);
    const bool desktop_exists = std::filesystem::exists(m_desktop_path);
    return !bin_exists && !desktop_exists;
}

}  // namespace malama::core
