#!/usr/bin/env bash

# This script will print any usages of IApplication - can be used to figure out
# dependencies of an file. e.g. if used on SeventvEmotes.cpp, you'll see it uses SeventvAPI

set -e

command_file="/tmp/c2-get-app-uses-command"

usage() {
    >&2 echo "usage: $0 <file> - prints the Application dependencies this file has"
    exit
}

file="$1"

if [ -z "$file" ]; then
    usage
fi

if [ ! -f "$file" ]; then
    >&2 echo "error: file '$file' does not exist"
    exit 1
fi

echo 'set output print
match cxxMemberCallExpr(on(hasType(asString("IApplication *"))))' >"$command_file"

next=0
usages=()
while read -r l; do
    # echo "l: '$l'"
    if [ "$next" = "1" ]; then
        echo "$l"
        usages+=("$l")
    fi
    next=0
    if [[ $l = 'Binding for "root":' ]]; then
        next=1
    fi
done <<< "$(clang-query "$file" -f="$command_file" 2>/dev/null)"
