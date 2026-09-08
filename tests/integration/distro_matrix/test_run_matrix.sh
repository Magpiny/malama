#!/usr/bin/env bash
# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/integration/distro_matrix/test_run_matrix.sh
# Purpose:     Static validation tests for run_matrix.sh and the three
#              Dockerfiles introduced in v0.0.4 cross-distribution build matrix
# Author:      Wanjare <wanjare@magpiny.dev>
# Created:     2026-06-08
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

# ---------------------------------------------------------------------------
# Resolve paths
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_MATRIX="${SCRIPT_DIR}/run_matrix.sh"

UBUNTU_DF="${SCRIPT_DIR}/ubuntu.dockerfile"
FEDORA_DF="${SCRIPT_DIR}/fedora.dockerfile"
ARCH_DF="${SCRIPT_DIR}/arch.dockerfile"

# ---------------------------------------------------------------------------
# Minimal test harness (no external dependencies)
# ---------------------------------------------------------------------------
PASS=0
FAIL=0
FAILURES=()

assert_contains() {
    local description="$1"
    local file="$2"
    local pattern="$3"

    if grep -qF -- "${pattern}" "${file}"; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s\n' "  => expected pattern not found: ${pattern}" >&2
        printf '  [FAIL] %s\n' "${description}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_not_contains() {
    local description="$1"
    local file="$2"
    local pattern="$3"

    if ! grep -qF -- "${pattern}" "${file}"; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s\n' "${description}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_file_exists() {
    local description="$1"
    local file="$2"

    if [[ -f "${file}" ]]; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — file not found: %s\n' "${description}" "${file}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_executable() {
    local description="$1"
    local file="$2"

    if [[ -x "${file}" ]]; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — not executable: %s\n' "${description}" "${file}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_file_absent() {
    local description="$1"
    local file="$2"

    if [[ ! -e "${file}" ]]; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — unexpected path exists: %s\n' "${description}" "${file}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_occurrences() {
    local description="$1"
    local file="$2"
    local pattern="$3"
    local expected="$4"
    local actual

    actual="$(grep -cF -- "${pattern}" "${file}" || true)"
    if [[ "${actual}" -eq "${expected}" ]]; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — expected %s occurrence(s), found %s: %s\n' \
            "${description}" "${expected}" "${actual}" "${pattern}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_exact_line() {
    local description="$1"
    local file="$2"
    local line="$3"

    if grep -qxF -- "${line}" "${file}"; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — exact line not found: %s\n' "${description}" "${line}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

assert_before() {
    local description="$1"
    local file="$2"
    local first_pattern="$3"
    local second_pattern="$4"
    local first_line
    local second_line

    first_line="$(grep -nF -- "${first_pattern}" "${file}" | head -n 1 | cut -d: -f1 || true)"
    second_line="$(grep -nF -- "${second_pattern}" "${file}" | head -n 1 | cut -d: -f1 || true)"

    if [[ -n "${first_line}" && -n "${second_line}" && "${first_line}" -lt "${second_line}" ]]; then
        printf '  [PASS] %s\n' "${description}"
        PASS=$(( PASS + 1 ))
    else
        printf '  [FAIL] %s — expected first pattern before second pattern\n' "${description}" >&2
        FAIL=$(( FAIL + 1 ))
        FAILURES+=("${description}")
    fi
}

# ---------------------------------------------------------------------------
# Suite 1: run_matrix.sh existence and permissions
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 1: run_matrix.sh file presence and permissions ==="

assert_file_exists \
    "run_matrix.sh exists at expected path" \
    "${RUN_MATRIX}"

assert_executable \
    "run_matrix.sh has executable permission" \
    "${RUN_MATRIX}"

# ---------------------------------------------------------------------------
# Suite 2: run_matrix.sh structural correctness
# Tests the key implementation decisions introduced in this PR.
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 2: run_matrix.sh structural content ==="

assert_contains \
    "run_matrix.sh enables set -euo pipefail for safe execution" \
    "${RUN_MATRIX}" \
    "set -euo pipefail"

assert_contains \
    "run_matrix.sh resolves SCRIPT_DIR from BASH_SOURCE[0]" \
    "${RUN_MATRIX}" \
    'SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"'

assert_contains \
    "run_matrix.sh derives PROJECT_ROOT relative to SCRIPT_DIR" \
    "${RUN_MATRIX}" \
    'PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"'

assert_contains \
    "run_matrix.sh defines DISTROS array" \
    "${RUN_MATRIX}" \
    'DISTROS=("arch" "fedora" "ubuntu")'

assert_contains \
    "run_matrix.sh iterates over distro array" \
    "${RUN_MATRIX}" \
    'for distro in "${DISTROS[@]}"; do'

assert_contains \
    "run_matrix.sh passes Dockerfile path using SCRIPT_DIR variable" \
    "${RUN_MATRIX}" \
    '-f "${SCRIPT_DIR}/${distro}.dockerfile"'

assert_contains \
    "run_matrix.sh tags images as malama-verify:\${distro}" \
    "${RUN_MATRIX}" \
    '-t "malama-verify:${distro}"'

assert_contains \
    "run_matrix.sh builds from PROJECT_ROOT context" \
    "${RUN_MATRIX}" \
    '"${PROJECT_ROOT}"'

assert_contains \
    "run_matrix.sh executes user-space installation test" \
    "${RUN_MATRIX}" \
    './install.sh'

assert_contains \
    "run_matrix.sh tests uninstallation with purge" \
    "${RUN_MATRIX}" \
    './uninstall.sh --purge'

assert_contains \
    "run_matrix.sh uses --rm flag to clean up containers after verification" \
    "${RUN_MATRIX}" \
    'docker run --rm'

# ---------------------------------------------------------------------------
# Suite 3: ubuntu.dockerfile content validation
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 3: ubuntu.dockerfile content ==="

assert_file_exists \
    "ubuntu.dockerfile exists" \
    "${UBUNTU_DF}"

assert_contains \
    "ubuntu.dockerfile uses pinned noble snapshot base image" \
    "${UBUNTU_DF}" \
    "FROM ubuntu:noble-20260509.1"

assert_contains \
    "ubuntu.dockerfile sets DEBIAN_FRONTEND=noninteractive to prevent interactive prompts" \
    "${UBUNTU_DF}" \
    "ENV DEBIAN_FRONTEND=noninteractive"

assert_contains \
    "ubuntu.dockerfile installs build-essential" \
    "${UBUNTU_DF}" \
    "build-essential"

assert_contains \
    "ubuntu.dockerfile installs cmake" \
    "${UBUNTU_DF}" \
    "cmake"

assert_contains \
    "ubuntu.dockerfile installs git" \
    "${UBUNTU_DF}" \
    "git"

assert_contains \
    "ubuntu.dockerfile installs libwxgtk3.2-dev for wxWidgets dependency" \
    "${UBUNTU_DF}" \
    "libwxgtk3.2-dev"

assert_contains \
    "ubuntu.dockerfile installs libboost-dev for Boost dependency" \
    "${UBUNTU_DF}" \
    "libboost-dev"

assert_contains \
    "ubuntu.dockerfile cleans apt cache to minimise image size" \
    "${UBUNTU_DF}" \
    "rm -rf /var/lib/apt/lists/*"

assert_contains \
    "ubuntu.dockerfile sets WORKDIR to /workspace" \
    "${UBUNTU_DF}" \
    "WORKDIR /workspace"

# ---------------------------------------------------------------------------
# Suite 4: fedora.dockerfile content validation
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 4: fedora.dockerfile content ==="

assert_file_exists \
    "fedora.dockerfile exists" \
    "${FEDORA_DF}"

assert_contains \
    "fedora.dockerfile uses fedora base image" \
    "${FEDORA_DF}" \
    "FROM fedora:"

assert_contains \
    "fedora.dockerfile installs gcc-c++" \
    "${FEDORA_DF}" \
    "gcc-c++"

assert_contains \
    "fedora.dockerfile installs cmake" \
    "${FEDORA_DF}" \
    "cmake"

assert_contains \
    "fedora.dockerfile installs git" \
    "${FEDORA_DF}" \
    "git"

assert_contains \
    "fedora.dockerfile installs wxGTK-devel for wxWidgets dependency" \
    "${FEDORA_DF}" \
    "wxGTK-devel"

assert_contains \
    "fedora.dockerfile installs boost-devel for Boost dependency" \
    "${FEDORA_DF}" \
    "boost-devel"

assert_contains \
    "fedora.dockerfile cleans dnf cache to minimise image size" \
    "${FEDORA_DF}" \
    "dnf clean all"

assert_contains \
    "fedora.dockerfile sets WORKDIR to /workspace" \
    "${FEDORA_DF}" \
    "WORKDIR /workspace"

# ---------------------------------------------------------------------------
# Suite 5: arch.dockerfile content validation
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 5: arch.dockerfile content ==="

assert_file_exists \
    "arch.dockerfile exists" \
    "${ARCH_DF}"

assert_contains \
    "arch.dockerfile uses archlinux base image" \
    "${ARCH_DF}" \
    "FROM archlinux:"

assert_contains \
    "arch.dockerfile enables parallel downloads in pacman.conf" \
    "${ARCH_DF}" \
    "ParallelDownloads"

assert_contains \
    "arch.dockerfile enables DisableDownloadTimeout for CI resilience" \
    "${ARCH_DF}" \
    "DisableDownloadTimeout"

assert_contains \
    "arch.dockerfile configures archive or mirror server" \
    "${ARCH_DF}" \
    "archive.archlinux.org"

assert_contains \
    "arch.dockerfile installs base-devel toolchain" \
    "${ARCH_DF}" \
    "base-devel"

assert_contains \
    "arch.dockerfile installs cmake" \
    "${ARCH_DF}" \
    "cmake"

assert_contains \
    "arch.dockerfile installs git" \
    "${ARCH_DF}" \
    "git"

assert_contains \
    "arch.dockerfile installs wxwidgets-gtk3 for wxWidgets dependency" \
    "${ARCH_DF}" \
    "wxwidgets-gtk3"

assert_contains \
    "arch.dockerfile installs boost for Boost dependency" \
    "${ARCH_DF}" \
    "boost"

assert_contains \
    "arch.dockerfile clears pacman cache to minimise image size" \
    "${ARCH_DF}" \
    "pacman -Scc --noconfirm"

assert_contains \
    "arch.dockerfile sets WORKDIR to /workspace" \
    "${ARCH_DF}" \
    "WORKDIR /workspace"

# ---------------------------------------------------------------------------
# Suite 6: .gitignore regression tests
# The PR diff adds build_arch, build_fedora, build_ubuntu to .gitignore so that
# the per-distro build directories created by run_matrix.sh are never tracked.
# ---------------------------------------------------------------------------
GITIGNORE="${SCRIPT_DIR}/../../../.gitignore"

echo ""
echo "=== Suite 6: .gitignore distro build directory entries ==="

assert_file_exists \
    ".gitignore exists at repository root" \
    "${GITIGNORE}"

assert_contains \
    ".gitignore excludes build_arch directory" \
    "${GITIGNORE}" \
    "build_arch"

assert_contains \
    ".gitignore excludes build_fedora directory" \
    "${GITIGNORE}" \
    "build_fedora"

assert_contains \
    ".gitignore excludes build_ubuntu directory" \
    "${GITIGNORE}" \
    "build_ubuntu"

# Cross-check: the run_matrix.sh build directory names must match the gitignore entries.
# If someone renames one in run_matrix.sh they must update .gitignore too.
for distro in ubuntu fedora arch; do
    assert_contains \
        "run_matrix.sh build dir 'build_${distro}' is present in .gitignore" \
        "${GITIGNORE}" \
        "build_${distro}"
done

# ---------------------------------------------------------------------------
# Suite 7: CMakeLists.txt version upgrade regression tests
# The PR diff bumped cmake_minimum_required from 4.2 to 3.28 (the actual
# minimum required), wxWidgets from 3.2 to 3.3, and added CMP0167 policy.
# ---------------------------------------------------------------------------
ROOT_CMAKE="${SCRIPT_DIR}/../../../CMakeLists.txt"

echo ""
echo "=== Suite 7: CMakeLists.txt version and policy assertions ==="

assert_file_exists \
    "Root CMakeLists.txt exists" \
    "${ROOT_CMAKE}"

assert_contains \
    "CMakeLists.txt requires cmake VERSION 3.28 (corrected from 4.2)" \
    "${ROOT_CMAKE}" \
    "cmake_minimum_required(VERSION 3.28)"

assert_not_contains \
    "CMakeLists.txt no longer references the incorrect cmake VERSION 4.2" \
    "${ROOT_CMAKE}" \
    "cmake_minimum_required(VERSION 4.2)"

assert_contains \
    "CMakeLists.txt requires wxWidgets 3.3 (bumped from 3.2)" \
    "${ROOT_CMAKE}" \
    "find_package(wxWidgets 3.2 REQUIRED"

assert_not_contains \
    "CMakeLists.txt no longer requires wxWidgets 3.3" \
    "${ROOT_CMAKE}" \
    "find_package(wxWidgets 3.3 REQUIRED"

assert_contains \
    "CMakeLists.txt sets CMP0167 policy to suppress FindBoost deprecation" \
    "${ROOT_CMAKE}" \
    "cmake_policy(SET CMP0167 NEW)"

assert_contains \
    "CMakeLists.txt guards CMP0167 policy with if(POLICY ...) check" \
    "${ROOT_CMAKE}" \
    "if(POLICY CMP0167)"

# ---------------------------------------------------------------------------
# Suite 8: PR #58 CMake packaging and test-gating regressions
# ---------------------------------------------------------------------------
echo ""
echo "=== Suite 8: PR #58 CMake packaging configuration ==="

assert_contains \
    "CMake aliases the distro-provided SQLite target when required" \
    "${ROOT_CMAKE}" \
    "if(TARGET SQLite::SQLite3 AND NOT TARGET SQLite3::SQLite3)"

assert_contains \
    "CMake creates the SQLite3 compatibility alias" \
    "${ROOT_CMAKE}" \
    "add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)"

assert_exact_line \
    "Tests are added only when BUILD_TESTING is enabled" \
    "${ROOT_CMAKE}" \
    "if(BUILD_TESTING)"

assert_not_contains \
    "Top-level builds no longer enable tests merely because malama is the root project" \
    "${ROOT_CMAKE}" \
    "if(BUILD_TESTING OR CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)"

assert_contains \
    "DEB packages declare the amd64 architecture" \
    "${ROOT_CMAKE}" \
    'set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")'

assert_contains \
    "DEB package names use CPack's distro-native default" \
    "${ROOT_CMAKE}" \
    "set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)"

assert_contains \
    "DEB packaging discovers shared-library dependencies" \
    "${ROOT_CMAKE}" \
    "set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)"

assert_not_contains \
    "DEB shared-library dependency discovery is not disabled" \
    "${ROOT_CMAKE}" \
    "set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)"

assert_contains \
    "RPM packages declare the x86_64 architecture" \
    "${ROOT_CMAKE}" \
    'set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")'

assert_contains \
    "RPM package names use CPack's distro-native default" \
    "${ROOT_CMAKE}" \
    "set(CPACK_RPM_FILE_NAME RPM-DEFAULT)"

# ---------------------------------------------------------------------------
# Suite 9: PR #58 Arch dependency metadata regressions
# ---------------------------------------------------------------------------
PKGBUILD_FILE="${SCRIPT_DIR}/../../../PKGBUILD/PKGBUILD"
SRCINFO_FILE="${SCRIPT_DIR}/../../../PKGBUILD/.SRCINFO"

echo ""
echo "=== Suite 9: PR #58 Arch package dependencies ==="

assert_contains \
    "PKGBUILD requires the Cobalt-compatible boost-libs release" \
    "${PKGBUILD_FILE}" \
    "  'boost-libs>=1.92.0'"

assert_not_contains \
    "PKGBUILD no longer leaves boost-libs unversioned" \
    "${PKGBUILD_FILE}" \
    "  'boost-libs'"

assert_contains \
    "PKGBUILD disables project tests for package builds" \
    "${PKGBUILD_FILE}" \
    "    -DBUILD_TESTING=OFF"

assert_contains \
    "PKGBUILD records the updated source archive checksum" \
    "${PKGBUILD_FILE}" \
    "sha256sums=('e1b03c3e96361107fc7a14db5084e401763702b8fc8f712a7ccfd7d85d680fc5')"

assert_file_absent \
    "Obsolete generated .SRCINFO metadata remains removed" \
    "${SRCINFO_FILE}"

assert_contains \
    "Arch matrix installs boost headers" \
    "${ARCH_DF}" \
    "    boost \\"

assert_contains \
    "Arch matrix separately installs the boost runtime libraries" \
    "${ARCH_DF}" \
    "    boost-libs \\"

# ---------------------------------------------------------------------------
# Suite 10: PR #58 AppImage icon layout regressions
# ---------------------------------------------------------------------------
APPIMAGE_SCRIPT="${SCRIPT_DIR}/../../../scripts/build_appimage.sh"

echo ""
echo "=== Suite 10: PR #58 AppImage icon layout ==="

assert_contains \
    "AppImage retains the freedesktop icon location" \
    "${APPIMAGE_SCRIPT}" \
    'cp assets/malama.png "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/malama.png"'

assert_contains \
    "AppImage creates the executable-relative assets directory" \
    "${APPIMAGE_SCRIPT}" \
    'mkdir -p "${APP_DIR}/usr/bin/assets"'

assert_contains \
    "AppImage copies the icon beside the executable" \
    "${APPIMAGE_SCRIPT}" \
    'cp assets/malama.png "${APP_DIR}/usr/bin/assets/malama.png"'

assert_before \
    "Executable-relative icon directory is created before its copy" \
    "${APPIMAGE_SCRIPT}" \
    'mkdir -p "${APP_DIR}/usr/bin/assets"' \
    'cp assets/malama.png "${APP_DIR}/usr/bin/assets/malama.png"'

# ---------------------------------------------------------------------------
# Suite 11: PR #58 application icon discovery regressions
# This anonymous UI helper cannot be linked into the Catch2 core target without
# pulling in the full wxWidgets frame, so validate each deployment branch and
# its priority order directly in the implementation source.
# ---------------------------------------------------------------------------
MAIN_FRAME_SOURCE="${SCRIPT_DIR}/../../../src/ui/main_frame.cpp"

echo ""
echo "=== Suite 11: PR #58 icon discovery paths and fallbacks ==="

assert_contains \
    "Icon discovery reads APPDIR" \
    "${MAIN_FRAME_SOURCE}" \
    'const char *appdir = std::getenv("APPDIR");'

assert_contains \
    "Empty APPDIR values are ignored" \
    "${MAIN_FRAME_SOURCE}" \
    "appdir != nullptr && appdir[0] != '\\0'"

assert_contains \
    "AppImage icon lookup uses APPDIR/usr/share" \
    "${MAIN_FRAME_SOURCE}" \
    'fs::path(appdir) / "usr" / kIconRelative'

assert_contains \
    "Installed icon lookup derives the FHS prefix from the executable" \
    "${MAIN_FRAME_SOURCE}" \
    "exe_dir.parent_path() / kIconRelative"

assert_contains \
    "Development icon lookup checks assets beside the executable" \
    "${MAIN_FRAME_SOURCE}" \
    'exe_dir / "assets" / "malama.png"'

assert_contains \
    "System icon lookup checks /usr/share" \
    "${MAIN_FRAME_SOURCE}" \
    'fs::path("/usr") / kIconRelative'

assert_contains \
    "Working-directory assets remain the final development fallback" \
    "${MAIN_FRAME_SOURCE}" \
    'candidates.emplace_back("assets/malama.png")'

assert_contains \
    "Filesystem lookup uses error codes inside the noexcept resolver" \
    "${MAIN_FRAME_SOURCE}" \
    "fs::exists(path, ec) && !ec"

assert_before \
    "APPDIR lookup has priority over installed-prefix lookup" \
    "${MAIN_FRAME_SOURCE}" \
    'fs::path(appdir) / "usr" / kIconRelative' \
    "exe_dir.parent_path() / kIconRelative"

assert_before \
    "Installed-prefix lookup has priority over executable assets" \
    "${MAIN_FRAME_SOURCE}" \
    "exe_dir.parent_path() / kIconRelative" \
    'exe_dir / "assets" / "malama.png"'

assert_before \
    "System icon lookup has priority over the working-directory fallback" \
    "${MAIN_FRAME_SOURCE}" \
    'fs::path("/usr") / kIconRelative' \
    'candidates.emplace_back("assets/malama.png")'

assert_occurrences \
    "The shared icon resolver is used by both frame and About dialog" \
    "${MAIN_FRAME_SOURCE}" \
    "const auto icon_path = resolve_icon_path();" \
    2

assert_contains \
    "MainFrame construction loads the application icon" \
    "${MAIN_FRAME_SOURCE}" \
    "    load_application_icon();"

# ---------------------------------------------------------------------------
# Suite 12: PR #58 cross-distro workflow regressions
# ---------------------------------------------------------------------------
WORKFLOW="${SCRIPT_DIR}/../../../.github/workflows/ci_cd.yml"

echo ""
echo "=== Suite 12: PR #58 release workflow ==="

assert_contains \
    "Debian packaging installs GLib development headers" \
    "${WORKFLOW}" \
    "libspdlog-dev libglib2.0-dev dpkg-dev"

assert_contains \
    "Fedora packaging installs GLib development headers" \
    "${WORKFLOW}" \
    "wxGTK-devel sqlite-devel poppler-cpp-devel glib2-devel"

assert_contains \
    "Debian packaging caches its Boost installation" \
    "${WORKFLOW}" \
    'key: boost-ubuntu-${{ env.BOOST_VERSION }}'

assert_contains \
    "Fedora packaging uses a distro-specific Boost cache" \
    "${WORKFLOW}" \
    'key: boost-fedora-${{ env.BOOST_VERSION }}'

assert_occurrences \
    "Both binary package jobs skip Boost compilation on a cache hit" \
    "${WORKFLOW}" \
    "if: steps.cache-boost.outputs.cache-hit != 'true'" \
    2

assert_occurrences \
    "Both binary package jobs build static Boost libraries" \
    "${WORKFLOW}" \
    "link=static threading=multi" \
    2

assert_occurrences \
    "Both binary package jobs ask CMake for static Boost libraries" \
    "${WORKFLOW}" \
    "-DBoost_USE_STATIC_LIBS=ON" \
    2

assert_not_contains \
    "Binary package jobs no longer build shared Boost libraries" \
    "${WORKFLOW}" \
    "link=shared threading=multi"

assert_contains \
    "Release publishing downloads artifacts into one directory" \
    "${WORKFLOW}" \
    "          path: release_assets"

assert_contains \
    "Release artifact downloads merge all packaging outputs" \
    "${WORKFLOW}" \
    "          merge-multiple: true"

assert_exact_line \
    "Release publishing requires a version tag, non-cancellation, and successful tests" \
    "${WORKFLOW}" \
    "    if: startsWith(github.ref, 'refs/tags/v') && !cancelled() && needs.build-test.result == 'success'"

# ---------------------------------------------------------------------------
# Results summary
# ---------------------------------------------------------------------------
echo ""
echo "============================================================"
echo "  Results: ${PASS} passed, ${FAIL} failed"
echo "============================================================"

if [[ ${FAIL} -gt 0 ]]; then
    echo ""
    echo "Failed tests:"
    for name in "${FAILURES[@]}"; do
        echo "  - ${name}"
    done
    echo ""
    exit 1
fi

echo "All tests passed."
exit 0
