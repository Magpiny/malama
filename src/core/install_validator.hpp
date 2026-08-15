/////////////////////////////////////////////////////////////////////////////
// Name:        src/core/install_validator.hpp
// Purpose:     Post-installation path and workspace permissions validator
// Author:      Wanjare S. (Magpiny)
// Created:     2026-08-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later

#include <expected>
#include <filesystem>

namespace malama::core {

enum class ValidationError : std::uint8_t {
    HomeDirNotFound,
    ConfigDirectoryUnwritable,
    BinaryNotInPath,
    DesktopEntryMissing
};

enum class ScrubMode : uint8_t { PreserveConfig, PurgeConfig };

class InstallValidator {
   public:
    explicit InstallValidator(std::filesystem::path home_override = {});

    [[nodiscard]] std::expected<void, ValidationError> validate_environment() const;
    [[nodiscard]] std::expected<void, ValidationError> verify_desktop_integration() const;
    [[nodiscard]] bool check_orphan_cleanup() const;

   private:
    std::filesystem::path m_home_path;
    std::filesystem::path m_bin_path;
    std::filesystem::path m_config_path;
    std::filesystem::path m_desktop_path;
};

}  // namespace malama::core
