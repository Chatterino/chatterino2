#!/usr/bin/env bash

set -eu

NUM_TIDY_JOBS=${NUM_TIDY_JOBS:-$(nproc)}

# example usage: FAST=1 ./tools/check-clang-tidy.sh --checks '-*,modernize-return-braced-init-list' --fix
# potentially easy fixes:
#  - readability-redundant-member-init
#  - readability-inconsistent-declaration-parameter-name
#  - readability-make-member-function-const
#  - cppcoreguidelines-misleading-capture-default-by-value
#  - disable readability-convert-member-functions-to-static

clang-tidy --version

if [ "$FAST" = "1" ]; then
    find src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \) | parallel --jobs "$NUM_TIDY_JOBS" --verbose clang-tidy --quiet "$@"
else
    while read -r file; do
        clang-tidy --quiet "$@" "$file"
    done < <(find src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))
fi
