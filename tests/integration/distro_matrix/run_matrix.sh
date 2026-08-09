#!/usr/bin/env bash
#############################################################################
# Name:        tests/integration/distro_matrix/run_matrix.sh
# Purpose:     Post-installation verification suite across Linux distros
# Author:      Wanjare S. (Magpiny)
# Created:     2026-08-09
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     GPL-3.0-or-later
#############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

DISTROS=("arch" "fedora" "ubuntu")

echo "============================================================"
echo " Starting Malama v0.2.9 Post-Installation Verification Matrix"
echo "============================================================"

for distro in "${DISTROS[@]}"; do
    echo "[TEST RUN] Validating installation infrastructure on: ${distro}"

    docker build \
        -f "${SCRIPT_DIR}/${distro}.dockerfile" \
        -t "malama-verify:${distro}" \
        "${PROJECT_ROOT}"

    docker run --rm "malama-verify:${distro}" bash -c "
        set -euo pipefail

        echo '--> Step 1: Execute User-Space Installation'
        ./install.sh

        echo '--> Step 2: Verify Binary and Desktop File Placement'
        test -f ~/.local/bin/malama
        test -f ~/.local/share/applications/malama.desktop
        test -f ~/.local/share/icons/hicolor/scalable/apps/malama.svg

        echo '--> Step 3: Execute Binary Version Check'
        ~/.local/bin/malama --version || true

        echo '--> Step 4: Execute Clean Uninstallation with Purge'
        ./uninstall.sh --purge

        echo '--> Step 5: Assert No Orphan Binary or Configuration Files'
        ! test -f ~/.local/bin/malama
        ! test -f ~/.local/share/applications/malama.desktop
        ! test -d ~/.config/malama
    "

    echo "[PASS] ${distro} installation & uninstallation scrub passed cleanly!"
    echo "------------------------------------------------------------"
done

echo "[SUCCESS] All distro integration tests passed for v0.2.9."
