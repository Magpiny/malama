#!/usr/bin/env bash
#############################################################################
# Name:        install.sh
# Purpose:     User-space installer for Malama desktop client
# Author:      Wanjare S. <samuelwanjare@protonmail.com>
# Created:     2026-08-09
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     GPL-3.0-or-later
#############################################################################

set -euo pipefail

BIN_DIR="${HOME}/.local/bin"
APP_DIR="${HOME}/.local/share/applications"
ICON_DIR="${HOME}/.local/share/icons/hicolor/scalable/apps"
CONFIG_DIR="${HOME}/.config/malama"

echo "[INFO] Installing Malama v0.2.9..."

mkdir -p "${BIN_DIR}" "${APP_DIR}" "${ICON_DIR}" "${CONFIG_DIR}"

if [[ ! -f "./build/malama" ]]; then
    echo "[ERROR] Binary ./build/malama not found. Build the project first." >&2
    exit 1
fi

# Copy application binary
install -m 755 "./build/malama" "${BIN_DIR}/malama"

# Install XDG Desktop Shortcut
if [[ -f "./assets/malama.desktop" ]]; then
    install -m 644 "./assets/malama.desktop" "${APP_DIR}/malama.desktop"
    if command -v desktop-file-validate &> /dev/null; then
        desktop-file-validate "${APP_DIR}/malama.desktop" || echo "[WARN] Desktop file validation failed."
    fi
fi

# Install Icon
if [[ -f "./assets/malama.svg" ]]; then
    install -m 644 "./assets/malama.svg" "${ICON_DIR}/malama.svg"
fi

# Refresh XDG desktop cache if available
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "${APP_DIR}" &> /dev/null || true
fi

echo "[SUCCESS] Malama installed successfully to ${BIN_DIR}/malama"
