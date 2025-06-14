#!/usr/bin/env bash

NUM_FORMAT_JOBS=${NUM_FORMAT_JOBS:-$(nproc)}

if ! command -v parallel >/dev/null 2>&1; then
    echo "Missing parallel command"
    exit 1
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Missing clang-format command"
    exit 1
fi

read -p "Are you sure you want to run clang-format on all source files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    find . \( \
        -regex '\./src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./tests/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./benchmarks/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./mocks/include/.*\.\(hpp\|cpp\)' \
        \) | parallel --verbose --jobs "$NUM_FORMAT_JOBS" clang-format -i
fi
