# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/integration/distro_matrix/arch.dockerfile
# Purpose:     Hardened reproducible Arch Linux workspace with explicit archive snapshots
# Author:      Wanjare <wanjare@magpiny.dev>
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

FROM archlinux:base-20260531.0.538839

# Configure pacman to download concurrently, never time out, and use the exact matching archive day
RUN sed -i 's/#ParallelDownloads = 5/ParallelDownloads = 5\nDisableDownloadTimeout/' /etc/pacman.conf \
    && echo "Server = https://archive.archlinux.org/repos/2026/05/01/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist

# Synchronize and lock the toolchain to the exact historical snapshot state
RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel \
    cmake \
    git \
    wxwidgets-gtk3 \
    boost \
    && pacman -Scc --noconfirm

WORKDIR /workspace
