#!/usr/bin/env bash
#############################################################################
# Name:        uninstall.sh
# Purpose:     Uninstaller and data scrubber for Malama
# Author:      Wanjare S. <samuelwanjare@protonmail.com>
# Created:     2026-08-09
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     GPL-3.0-or-later
#############################################################################

set -euo pipefail

BIN_PATH="${HOME}/.local/bin/malama"
APP_PATH="${HOME}/.local/share/applications/malama.desktop"
ICON_PATH="${HOME}/.local/share/icons/hicolor/scalable/apps/malama.svg"
CONFIG_DIR="${HOME}/.config/malama"

PURGE_DATA=false

for arg in "$@"; do
    if [[ "${arg}" == "--purge" ]]; then
        PURGE_DATA=true
    fi
done

echo "[INFO] Removing Malama binary and desktop shortcuts..."

rm -f "${BIN_PATH}"
rm -f "${APP_PATH}"
rm -f "${ICON_PATH}"

if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "${HOME}/.local/share/applications" &> /dev/null || true
fi

if [[ "${PURGE_DATA}" == true ]]; then
    echo "[INFO] Scrubbing persistent configuration and database in ${CONFIG_DIR}..."
    rm -rf "${CONFIG_DIR}"
fi

echo "[SUCCESS] Malama uninstalled cleanly."
