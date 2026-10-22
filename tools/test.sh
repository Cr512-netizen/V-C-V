#!/usr/bin/env sh
set -eu
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DNUC_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
