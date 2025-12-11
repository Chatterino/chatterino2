#!/usr/bin/env bash

read -p "Are you sure you want to run clang-format on all source files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [ "$FAST" = "1" ]; then
        if ! command -v parallel >/dev/null 2>&1; then
            echo "Missing parallel command"
            exit 1
        fi

        NUM_FORMAT_JOBS=${NUM_FORMAT_JOBS:-$(nproc)}

        echo "Running clang-format with ${NUM_FORMAT_JOBS} jobs"

        find . \( \
            -regex '\./src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./tests/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./benchmarks/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./lib/twitch-eventsub-ws/include/.*\.\(hpp\|cpp\)' -o \
            -regex '\./lib/twitch-eventsub-ws/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./mocks/include/.*\.\(hpp\|cpp\)' \
            \) | parallel --verbose --jobs "$NUM_FORMAT_JOBS" clang-format -i
    else
        find . \( \
            -regex '\./src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./tests/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./benchmarks/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./lib/twitch-eventsub-ws/include/.*\.\(hpp\|cpp\)' -o \
            -regex '\./lib/twitch-eventsub-ws/src/.*\.\(hpp\|cpp\)' -o \
            -regex '\./mocks/include/.*\.\(hpp\|cpp\)' \
            \) -exec clang-format -i {} \;
    fi
fi
