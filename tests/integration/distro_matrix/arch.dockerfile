FROM archlinux:latest

# Optimize pacman pipeline for connection resilience, multi-threading, and infinite timeout clearance
RUN sed -i 's/#ParallelDownloads = 5/ParallelDownloads = 5\nDisableDownloadTimeout/' /etc/pacman.conf \
    && echo "Server = https://geo.mirror.pkgbuild.com/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist \
    && echo "Server = https://mirror.rackspace.com/archlinux/\$repo/os/\$arch" >> /etc/pacman.d/mirrorlist

# Synchronize the package databases and install our strict testing target suite
RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel cmake git wxwidgets-gtk3 boost \
    && pacman -Scc --noconfirm

WORKDIR /workspace
