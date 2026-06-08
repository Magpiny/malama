FROM fedora:40
RUN dnf update -y && dnf install -y \
    gcc-c++ cmake git wxGTK-devel boost-devel \
    && dnf clean all
WORKDIR /workspace
