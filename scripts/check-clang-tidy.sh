#!/usr/bin/env bash

set -eu

clang-tidy --version

# clang-tidy --checks '-*,readability-braces-around-statements' src/RunGui.cpp
while read -r file; do
    echo "Running clang-tidy on $file"
    clang-tidy --quiet "$@" "$file"
done < <(find src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))

while read -r file; do
    echo "Running clang-tidy on $file"
    clang-tidy --quiet "$@" "$file"
done < <(find tests/src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))

while read -r file; do
    echo "Running clang-tidy on $file"
    clang-tidy --quiet "$@" "$file"
done < <(find benchmarks/src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))

while read -r file; do
    echo "Running clang-tidy on $file"
    clang-tidy --quiet "$@" "$file"
done < <(find mocks/include/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))
