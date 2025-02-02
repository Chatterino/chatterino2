#!/usr/bin/env python3

import logging
import os
import re
import sys
from os.path import realpath

from lib import (
    definition_markers,
    generate,
    implementation_markers,
    init_clang_cindex,
    init_logging,
    replace_in_file,
)

log = logging.getLogger("generate-and-replace-dir")


def main():
    init_logging()

    init_clang_cindex()

    if len(sys.argv) < 2:
        log.error(f"usage: {sys.argv[0]} <path-to-dir>")
        sys.exit(1)

    additional_includes = []
    if len(sys.argv) >= 4 and sys.argv[2] == "--includes":
        additional_includes = sys.argv[3].split(";")
        log.debug(f"additional includes: {additional_includes}")

    dir = realpath(sys.argv[1])
    log.debug(f"Searching for header files in {dir}")

    for path in os.scandir(dir):
        if not path.is_file():
            continue

        if not path.name.endswith(".hpp"):
            continue

        header_path = path.path
        source_path = re.sub(r"\.hpp$", ".cpp", header_path)
        source_path = re.sub(r"include[/\\]twitch-eventsub-ws[/\\]", "src/", source_path)

        if not os.path.isfile(source_path):
            log.warning(f"Header file {header_path} did not have a matching source file at {source_path}")
            continue

        log.debug(f"Found header & source {header_path} / {source_path}")

        (definition, implementation) = generate(header_path, additional_includes)

        replace_in_file(header_path, definition_markers, definition)
        replace_in_file(source_path, implementation_markers, implementation)


if __name__ == "__main__":
    main()
