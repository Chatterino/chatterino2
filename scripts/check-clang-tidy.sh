#!/usr/bin/env bash

set -eu

clang-tidy --version

find \
    src/ \
    tests/src/ \
    benchmarks/src/ \
    mocks/include/ \
    -type f \( -name "*.hpp" -o -name "*.cpp" \) -print0 | parallel -0 -j16 -I {} clang-tidy --quiet "$@" "{}"
