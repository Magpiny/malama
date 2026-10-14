// /////////////////////////////////////////////////////////////////////////////
// Name:        src/ui/icon_path_resolver.hpp
// Purpose:     Resolves the application icon across supported deployment layouts
// Author:      Wanjare S. <samuelwanjare@proton.me>
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <system_error>
#include <vector>

namespace malama::ui::detail {

/// Returns the first regular-file icon in deployment-priority order.
/// Empty roots are omitted so unavailable runtime context cannot produce
/// accidental relative-path candidates.
[[nodiscard]] inline std::filesystem::path resolve_icon_path(
    const std::filesystem::path &app_dir, const std::filesystem::path &executable_path,
    const std::filesystem::path &system_prefix,
    const std::filesystem::path &working_directory) noexcept {
    namespace fs = std::filesystem;
    constexpr const char *kIconRelative = "share/icons/hicolor/256x256/apps/malama.png";

    std::vector<fs::path> candidates;

    if (!app_dir.empty()) {
        candidates.emplace_back(app_dir / "usr" / kIconRelative);
    }

    if (!executable_path.empty()) {
        const fs::path executable_dir = executable_path.parent_path();
        candidates.emplace_back(executable_dir.parent_path() / kIconRelative);
        candidates.emplace_back(executable_dir / "assets" / "malama.png");
    }

    if (!system_prefix.empty()) {
        candidates.emplace_back(system_prefix / kIconRelative);
    }

    if (!working_directory.empty()) {
        candidates.emplace_back(working_directory / "assets" / "malama.png");
    }

    for (const auto &candidate : candidates) {
        std::error_code error;
        if (fs::is_regular_file(candidate, error) && !error) {
            return candidate;
        }
    }

    return {};
}

}  // namespace malama::ui::detail
