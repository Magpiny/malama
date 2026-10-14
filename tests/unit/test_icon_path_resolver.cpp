// /////////////////////////////////////////////////////////////////////////////
// Name:        tests/unit/test_icon_path_resolver.cpp
// Purpose:     Unit tests for deployment-aware application icon discovery
// Author:      Wanjare S. <samuelwanjare@protonmail.com>
// Copyright:   (c) 2026 Magpiny. All rights reserved.
// Licence:     GPL-3.0-or-later
// /////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "ui/icon_path_resolver.hpp"

namespace {

namespace fs = std::filesystem;

class TemporaryTree final {
   public:
    TemporaryTree()
        : m_root(fs::temp_directory_path() /
                 ("malama_icon_resolver_" +
                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        fs::create_directories(m_root);
    }

    ~TemporaryTree() { fs::remove_all(m_root); }

    TemporaryTree(const TemporaryTree &) = delete;
    TemporaryTree &operator=(const TemporaryTree &) = delete;

    [[nodiscard]] const fs::path &root() const noexcept { return m_root; }

    [[nodiscard]] fs::path create_file(const fs::path &relative_path) const {
        const fs::path file_path = m_root / relative_path;
        fs::create_directories(file_path.parent_path());
        std::ofstream(file_path) << "icon";
        return file_path;
    }

   private:
    fs::path m_root;
};

constexpr const char *kInstalledIcon = "share/icons/hicolor/256x256/apps/malama.png";

}  // namespace

TEST_CASE("Icon resolver returns no path when no icon exists", "[unit][ui][icon]") {
    const TemporaryTree tree;

    const fs::path result = malama::ui::detail::resolve_icon_path(
        tree.root() / "appdir", tree.root() / "prefix/bin/malama", tree.root() / "system",
        tree.root() / "working");

    REQUIRE(result.empty());
}

TEST_CASE("Icon resolver honors deployment search priority", "[unit][ui][icon]") {
    const TemporaryTree tree;
    const fs::path app_dir = tree.root() / "appdir";
    const fs::path executable = tree.root() / "prefix/bin/malama";
    const fs::path system_prefix = tree.root() / "system";
    const fs::path working_directory = tree.root() / "working";

    const fs::path appimage_icon = tree.create_file("appdir/usr" / fs::path(kInstalledIcon));
    const fs::path fhs_icon = tree.create_file("prefix" / fs::path(kInstalledIcon));
    const fs::path adjacent_icon = tree.create_file("prefix/bin/assets/malama.png");
    const fs::path system_icon = tree.create_file("system" / fs::path(kInstalledIcon));
    const fs::path working_icon = tree.create_file("working/assets/malama.png");

    SECTION("AppImage icon wins over every fallback") {
        REQUIRE(malama::ui::detail::resolve_icon_path(app_dir, executable, system_prefix,
                                                       working_directory) == appimage_icon);
    }

    SECTION("FHS icon wins when AppImage context is unavailable") {
        REQUIRE(malama::ui::detail::resolve_icon_path({}, executable, system_prefix,
                                                       working_directory) == fhs_icon);
    }

    SECTION("Executable-adjacent asset wins when installed icons are absent") {
        fs::remove(appimage_icon);
        fs::remove(fhs_icon);
        REQUIRE(malama::ui::detail::resolve_icon_path(app_dir, executable, system_prefix,
                                                       working_directory) == adjacent_icon);
    }

    SECTION("System icon wins over the working-directory fallback") {
        REQUIRE(malama::ui::detail::resolve_icon_path({}, {}, system_prefix, working_directory) ==
                system_icon);
    }

    SECTION("Working-directory asset is the final fallback") {
        REQUIRE(malama::ui::detail::resolve_icon_path({}, {}, {}, working_directory) ==
                working_icon);
    }
}

TEST_CASE("Icon resolver skips unavailable higher-priority candidates", "[unit][ui][icon]") {
    const TemporaryTree tree;
    const fs::path executable = tree.root() / "prefix/bin/malama";
    const fs::path adjacent_icon = tree.create_file("prefix/bin/assets/malama.png");

    const fs::path result = malama::ui::detail::resolve_icon_path(
        tree.root() / "missing appdir", executable, tree.root() / "missing system",
        tree.root() / "missing working directory");

    REQUIRE(result == adjacent_icon);
}

TEST_CASE("Icon resolver skips a directory masquerading as an icon",
          "[unit][ui][icon][regression]") {
    const TemporaryTree tree;
    const fs::path app_dir = tree.root() / "appdir";
    fs::create_directories(app_dir / "usr" / kInstalledIcon);
    const fs::path fallback_icon = tree.create_file("working/assets/malama.png");

    const fs::path result =
        malama::ui::detail::resolve_icon_path(app_dir, {}, {}, tree.root() / "working");

    REQUIRE(result == fallback_icon);
}

TEST_CASE("Icon resolver handles empty runtime paths without relative leakage",
          "[unit][ui][icon][regression]") {
    const TemporaryTree tree;
    const fs::path unrelated_icon = tree.create_file(kInstalledIcon);

    const fs::path result = malama::ui::detail::resolve_icon_path({}, {}, {}, {});

    REQUIRE(fs::exists(unrelated_icon));
    REQUIRE(result.empty());
}

TEST_CASE("Icon resolver supports deployment roots containing spaces", "[unit][ui][icon]") {
    const TemporaryTree tree;
    const fs::path app_dir = tree.root() / "mounted App Image";
    const fs::path expected_icon =
        tree.create_file("mounted App Image/usr" / fs::path(kInstalledIcon));

    const fs::path result = malama::ui::detail::resolve_icon_path(app_dir, {}, {}, {});

    REQUIRE(result == expected_icon);
}
