#!/usr/bin/env sh
# Build with a CMake preset. Usage: tools/build.sh [debug|release]
set -eu
preset="${1:-debug}"
cmake --preset "$preset"
cmake --build --preset "$preset" --parallel
