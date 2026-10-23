#!/usr/bin/env sh
# Configure, build and run the whole test suite. Usage: tools/test.sh [debug|release]
set -eu
preset="${1:-debug}"
cmake --preset "$preset"
cmake --build --preset "$preset" --parallel
ctest --preset "$preset"
