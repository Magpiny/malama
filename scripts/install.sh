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
ICON_DIR="${HOME}/.local/share/icons/hicolor/256x256/apps"
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
# NOTE: the .desktop file is CMake-generated from assets/malama.desktop.in
# into build/malama.desktop (see configure_file() in CMakeLists.txt) -- the
# same source build_appimage.sh uses. assets/ itself may only hold the .in
# template, so check both locations and prefer the generated one.
DESKTOP_SRC=""
if [[ -f "./build/malama.desktop" ]]; then
    DESKTOP_SRC="./build/malama.desktop"
elif [[ -f "./assets/malama.desktop" ]]; then
    DESKTOP_SRC="./assets/malama.desktop"
fi
if [[ -n "${DESKTOP_SRC}" ]]; then
    install -m 644 "${DESKTOP_SRC}" "${APP_DIR}/malama.desktop"
    if command -v desktop-file-validate &> /dev/null; then
        desktop-file-validate "${APP_DIR}/malama.desktop" || echo "[WARN] Desktop file validation failed."
    fi
else
    echo "[WARN] No malama.desktop found in ./build/ or ./assets/ -- app will not appear in the menu."
    echo "[WARN] Run 'cmake -B build && cmake --build build' first to generate it."
fi
# Install Icons (PNG + SVG) -- ship both when available; icon themes prefer
# SVG for scalability, PNG remains as the raster fallback for anything that
# only indexes fixed-size directories.
ICON_INSTALLED=0
if [[ -f "./assets/malama.png" ]]; then
    install -m 644 "./assets/malama.png" "${ICON_DIR}/malama.png"
    ICON_INSTALLED=1
fi
if [[ -f "./assets/malama.svg" ]]; then
    SVG_ICON_DIR="${HOME}/.local/share/icons/hicolor/scalable/apps"
    mkdir -p "${SVG_ICON_DIR}"
    install -m 644 "./assets/malama.svg" "${SVG_ICON_DIR}/malama.svg"
    ICON_INSTALLED=1
fi
if [[ "${ICON_INSTALLED}" -eq 0 ]]; then
    echo "[WARN] No icon found in ./assets/ (expected malama.png and/or malama.svg) -- menu entry may show no icon or be hidden by some DEs."
fi
# Refresh XDG desktop cache if available
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "${APP_DIR}" 2>/dev/null || echo "[WARN] update-desktop-database reported an issue (non-fatal)."
fi
# Refresh icon cache if available
if command -v gtk-update-icon-cache &> /dev/null; then
    gtk-update-icon-cache -f -t "${HOME}/.local/share/icons/hicolor" 2>/dev/null || true
fi
# Refresh KDE Plasma's system config cache (kbuildsycoca) -- required on
# Plasma for new .desktop entries to appear in the application menu / KRunner.
# Only run this on an actual KDE session; it's a no-op (or misleading warning)
# on GNOME, XFCE, and other desktop environments.
CURRENT_DE="${XDG_CURRENT_DESKTOP:-}${DESKTOP_SESSION:-}"
if [[ "${CURRENT_DE,,}" == *kde* ]] || [[ "${CURRENT_DE,,}" == *plasma* ]]; then
    if command -v kbuildsycoca6 &> /dev/null; then
        kbuildsycoca6 --noincremental 2>/dev/null || echo "[WARN] kbuildsycoca6 refresh reported an issue (non-fatal)."
    elif command -v kbuildsycoca5 &> /dev/null; then
        kbuildsycoca5 --noincremental 2>/dev/null || echo "[WARN] kbuildsycoca5 refresh reported an issue (non-fatal)."
    fi
fi
# Ensure ~/.local/bin is on PATH -- warn if not, since `malama` won't run by
# bare name from a terminal otherwise (menu launch is unaffected by this).
case ":${PATH}:" in
    *":${BIN_DIR}:"*) ;;
    *) echo "[WARN] ${BIN_DIR} is not on your PATH. Add it to your shell profile to run 'malama' directly from a terminal." ;;
esac
echo "[SUCCESS] Malama installed. You may need to log out/in or restart Plasma for the menu entry to appear if it doesn't show immediately."
