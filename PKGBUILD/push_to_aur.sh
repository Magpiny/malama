#!/usr/bin/env bash
#############################################################################
# Name:        PKGBUILD/push_to_aur.sh
# Purpose:     Automates AUR package update, .SRCINFO generation and git push
# Author:      Magpiny <magpinyb@proton.me>
# Licence:     GPL-3.0-or-later
#############################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "🔵 [ℹ️ INFO] => Updating checksums and generating .SRCINFO..."
updpkgsums 2>/dev/null || true
makepkg --printsrcinfo > .SRCINFO

echo "🔵 [ℹ️ INFO] => .SRCINFO generated successfully:"
cat .SRCINFO

echo ""
echo "🔵 [ℹ️ INFO] => To publish or update Malama on AUR:"
echo "1. Clone your AUR repository if not already cloned:"
echo "   git clone ssh://aur@aur.archlinux.org/malama.git /tmp/aur-malama"
echo ""
echo "2. Copy PKGBUILD and .SRCINFO into the AUR repository:"
echo "   cp ${SCRIPT_DIR}/PKGBUILD ${SCRIPT_DIR}/.SRCINFO /tmp/aur-malama/"
echo ""
echo "3. Commit and push from the AUR repository:"
echo "   cd /tmp/aur-malama"
echo "   git add PKGBUILD .SRCINFO"
echo "   git commit -m 'release: bump malama to v\$(grep pkgver PKGBUILD | cut -d= -f2)'"
echo "   git push origin master"
echo ""
