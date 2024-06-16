#!/usr/bin/env bash

dependencies="$(jq -r -c '.dependencies[] | if type == "string" then . else .name end' vcpkg.json)"
dependencies+=" openssl"
baseline="$(jq -r -c '."builtin-baseline"' vcpkg.json)"

for dependency_name in $dependencies; do
    dependency_url="https://raw.githubusercontent.com/microsoft/vcpkg/${baseline}/ports/${dependency_name}/vcpkg.json"
    dependency_version="$(curl -s "$dependency_url" | jq -rc '.version')"
    echo "Dependency $dependency_name is at version '$dependency_version' in baseline $baseline"
done
