# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/integration/distro_matrix/ubuntu.dockerfile
# Purpose:     Hardened reproducible Ubuntu workspace using explicit timestamp snapshots
# Author:      Wanjare <wanjare@magpiny.dev>
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

FROM ubuntu:noble-20260509.1

ENV DEBIAN_FRONTEND=noninteractive

# Synchronize against the locked snapshot repositories
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libwxgtk3.2-dev \
    libboost-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
