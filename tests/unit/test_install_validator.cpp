/////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_install_validator.cpp
// Purpose:     Catch2 unit test cases for InstallValidator system checks
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Created:     2026-08-09
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "core/install_validator.hpp"

TEST_CASE("InstallValidator detects environment and path states", "[install][unit]") {
    const auto temp_home = std::filesystem::temp_directory_path() / "malama_test_home";
    std::filesystem::create_directories(temp_home / ".local" / "bin");
    std::filesystem::create_directories(temp_home / ".local" / "share" / "applications");

    malama::core::InstallValidator validator(temp_home);

    SECTION("Fails validation when binary and desktop entries are missing") {
        auto res = validator.verify_desktop_integration();
        REQUIRE_FALSE(res.has_value());
        REQUIRE(res.error() == malama::core::ValidationError::BinaryNotInPath);
    }

    SECTION("Passes validation when mock files are present") {
        std::ofstream(temp_home / ".local" / "bin" / "malama") << "#!/bin/sh\n";
        std::ofstream(temp_home / ".local" / "share" / "applications" / "malama.desktop")
            << "[Desktop Entry]\n";

        auto res = validator.verify_desktop_integration();
        REQUIRE(res.has_value());

        // Test orphan cleanup detector
        REQUIRE_FALSE(validator.check_orphan_cleanup());

        // Remove files and recheck
        std::filesystem::remove(temp_home / ".local" / "bin" / "malama");
        std::filesystem::remove(temp_home / ".local" / "share" / "applications" / "malama.desktop");
        REQUIRE(validator.check_orphan_cleanup());
    }

    std::filesystem::remove_all(temp_home);
}
