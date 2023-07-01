#!/usr/bin/env bash

set -eu

clang-tidy --version

find src/ -type f \( -name "*.hpp" -o -name "*.cpp" \) -print0 | parallel -0 -I {} clang-tidy --quiet "$@" "{}"
