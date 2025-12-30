#!/usr/bin/env bash

# example usage: FAST=1 ./scripts/check-clang-tidy.sh --checks '-*,modernize-return-braced-init-list' --fix

set -e

clang-tidy --version

if [ "$FAST" = "1" ]; then
    if ! command -v parallel >/dev/null 2>&1; then
        echo "Missing parallel command"
        exit 1
    fi

    NUM_TIDY_JOBS=${NUM_TIDY_JOBS:-$(nproc)}

    echo "Running clang-tidy with args '$*', with ${NUM_TIDY_JOBS} jobs"

    find . \( \
        -regex '\./src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./tests/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./benchmarks/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./mocks/include/.*\.\(hpp\|cpp\)' -o \
        -regex '\./lib/twitch-eventsub-ws/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./lib/twitch-eventsub-ws/include/.*\.\(hpp\|cpp\)' \
        \) | parallel --jobs "$NUM_TIDY_JOBS" --verbose clang-tidy --quiet "$@"
else
    find . \( \
        -regex '\./src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./tests/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./benchmarks/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./mocks/include/.*\.\(hpp\|cpp\)' -o \
        -regex '\./lib/twitch-eventsub-ws/src/.*\.\(hpp\|cpp\)' -o \
        -regex '\./lib/twitch-eventsub-ws/include/.*\.\(hpp\|cpp\)' \
        \) -exec clang-tidy --quiet "$@" {} \;
fi
