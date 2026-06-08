# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/integration/distro_matrix/fedora.dockerfile
# Purpose:     Hardened reproducible Fedora workspace using explicit timestamp snapshots
# Author:      Wanjare <wanjare@magpiny.dev>
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

FROM fedora:45

# Provision the explicit development packages inside the locked layer
RUN dnf update -y && dnf install -y \
    gcc-c++ \
    cmake \
    git \
    wxGTK-devel \
    boost-devel \
    && dnf clean all

WORKDIR /workspace
