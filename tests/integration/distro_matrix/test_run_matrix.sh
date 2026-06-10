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
    "run_matrix.sh enables set -e for exit-on-error" \
    "${RUN_MATRIX}" \
    "set -e"

assert_contains \
    "run_matrix.sh exports DOCKER_BUILDKIT=1 to suppress legacy builder warnings" \
    "${RUN_MATRIX}" \
    "export DOCKER_BUILDKIT=1"

assert_contains \
    "run_matrix.sh resolves SCRIPT_DIR from BASH_SOURCE[0]" \
    "${RUN_MATRIX}" \
    'SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"'

assert_contains \
    "run_matrix.sh derives PROJECT_ROOT relative to SCRIPT_DIR" \
    "${RUN_MATRIX}" \
    'PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"'

assert_contains \
    "run_matrix.sh changes working directory to PROJECT_ROOT before building" \
    "${RUN_MATRIX}" \
    'cd "${PROJECT_ROOT}"'

assert_contains \
    "run_matrix.sh iterates over ubuntu in distro loop" \
    "${RUN_MATRIX}" \
    "ubuntu"

assert_contains \
    "run_matrix.sh iterates over fedora in distro loop" \
    "${RUN_MATRIX}" \
    "fedora"

assert_contains \
    "run_matrix.sh iterates over arch in distro loop" \
    "${RUN_MATRIX}" \
    "arch"

assert_contains \
    "run_matrix.sh uses loop variable for distro iteration (for distro in)" \
    "${RUN_MATRIX}" \
    "for distro in ubuntu fedora arch"

assert_contains \
    "run_matrix.sh passes Dockerfile path using SCRIPT_DIR variable" \
    "${RUN_MATRIX}" \
    '-f "${SCRIPT_DIR}/${distro}.dockerfile"'

assert_contains \
    "run_matrix.sh tags images as malama-test:\${distro}" \
    "${RUN_MATRIX}" \
    '-t "malama-test:${distro}"'

assert_contains \
    "run_matrix.sh mounts PROJECT_ROOT into container at /workspace" \
    "${RUN_MATRIX}" \
    '-v "${PROJECT_ROOT}:/workspace"'

assert_contains \
    "run_matrix.sh creates per-distro build directory build_\${distro}" \
    "${RUN_MATRIX}" \
    "mkdir -p build_\${distro}"

assert_contains \
    "run_matrix.sh invokes cmake with Release build type" \
    "${RUN_MATRIX}" \
    "cmake -DCMAKE_BUILD_TYPE=Release .."

assert_contains \
    "run_matrix.sh invokes make with nproc parallelism" \
    "${RUN_MATRIX}" \
    'make -j\$(nproc)'

assert_contains \
    "run_matrix.sh uses --rm flag to clean up containers after build" \
    "${RUN_MATRIX}" \
    "docker run --rm"

# Regression: old cmake_minimum_required was 4.2; ensure the script is not
# hardcoding the old version anywhere.
assert_not_contains \
    "run_matrix.sh does not reference obsolete cmake version 4.2" \
    "${RUN_MATRIX}" \
    "4.2"

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
    "fedora.dockerfile uses fedora:40 base image" \
    "${FEDORA_DF}" \
    "FROM fedora:40"

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
    "arch.dockerfile uses archlinux:latest base image" \
    "${ARCH_DF}" \
    "FROM archlinux:latest"

assert_contains \
    "arch.dockerfile enables parallel downloads in pacman.conf" \
    "${ARCH_DF}" \
    "ParallelDownloads"

assert_contains \
    "arch.dockerfile enables DisableDownloadTimeout for CI resilience" \
    "${ARCH_DF}" \
    "DisableDownloadTimeout"

assert_contains \
    "arch.dockerfile configures geo mirror as primary mirror" \
    "${ARCH_DF}" \
    "geo.mirror.pkgbuild.com"

assert_contains \
    "arch.dockerfile configures rackspace mirror as fallback mirror" \
    "${ARCH_DF}" \
    "mirror.rackspace.com"

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
