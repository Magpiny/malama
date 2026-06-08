#!/usr/bin/env bash
# /////////////////////////////////////////////////////////////////////////////
# Name:        tests/integration/distro_matrix/run_matrix.sh
# Purpose:     Automated cross-distribution containerized build orchestration
# Author:      Wanjare <wanjare@magpiny.dev>
# Created:     2026-06-08
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     Apache-2.0
# /////////////////////////////////////////////////////////////////////////////

set -e

# Enforce BuildKit to eliminate legacy builder deprecation warnings
export DOCKER_BUILDKIT=1

# Resolve canonical paths dynamically relative to the script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

cd "${PROJECT_ROOT}"

echo "🚀 Starting malama Cross-Distribution Compilation Matrix Sweeps..."

for distro in ubuntu fedora arch; do
    echo "--------------------------------------------------------"
    echo "📦 Testing Environment Build Target: ${distro}"
    echo "--------------------------------------------------------"
    
    # Target the Dockerfiles using the dynamically calculated script directory path
    docker build \
        -t "malama-test:${distro}" \
        -f "${SCRIPT_DIR}/${distro}.dockerfile" \
        .
    
    # Mount workspace into container to compile and evaluate warnings
    docker run --rm -v "${PROJECT_ROOT}:/workspace" "malama-test:${distro}" bash -c "
        mkdir -p build_${distro} && cd build_${distro} && \
        cmake -DCMAKE_BUILD_TYPE=Release .. && \
        make -j\$(nproc)
    "
    echo "✅ Target environment ${distro} compiled cleanly with ZERO errors."
done

echo "🎉 All distribution validation sweeps completed successfully!"
