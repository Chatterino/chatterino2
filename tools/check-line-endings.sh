#!/bin/bash

set -eu

fail="0"

dos2unix --version

while read -r file; do
    num_dos_line_endings=$(dos2unix -id "$file" | awk '/[0-9]+/{print $(NF-1)}')
    if [ "$num_dos_line_endings" -gt "0" ]; then
        >&2 echo "File '$file' contains $num_dos_line_endings DOS line-endings, it should only be using unix line-endings!"
        fail="1"
    fi
done < <(find src/ -type f \( -iname "*.hpp" -o -iname "*.cpp" \))

if [ "$fail" = "1" ]; then
    >&2 echo "At least one file is not using unix line-endings - check the output above"
    exit 1
fi

>&2 echo "Every file seems to be using unix line-endings. Good job!"
