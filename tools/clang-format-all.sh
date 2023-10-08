#!/bin/bash

read -p "Are you sure you want to run clang-format on all source files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    find src/ \( -iname "*.hpp" -o -iname "*.cpp" \) -exec clang-format -i {} \;
    find tests/src/ \( -iname "*.hpp" -o -iname "*.cpp" \) -exec clang-format -i {} \;
    find benchmarks/src/ \( -iname "*.hpp" -o -iname "*.cpp" \) -exec clang-format -i {} \;
    find mocks/include/ \( -iname "*.hpp" -o -iname "*.cpp" \) -exec clang-format -i {} \;
fi
