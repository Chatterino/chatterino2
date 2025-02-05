#!/usr/bin/env python3

import logging
import sys
from os.path import realpath

from lib import definition_markers, implementation_markers, init_logging, replace_in_file

log = logging.getLogger("replace-in-file")


def main():
    init_logging()

    if len(sys.argv) < 3:
        log.error(f"usage: {sys.argv[0]} <path-to-header-file> <path-to-source-file>")
        sys.exit(1)

    # The files where we will do the replacements
    header_path = realpath(sys.argv[1])
    source_path = realpath(sys.argv[2])

    # The files where we read the generated definitions & implementations from
    # TODO: pass these along in argv too?
    stdin_lines = sys.stdin.readlines()
    generated_definition_path = realpath(stdin_lines[0].strip())
    generated_implementation_path = realpath(stdin_lines[1].strip())

    log.debug(f"Generated files: {generated_definition_path} / {generated_implementation_path}")
    log.debug(f"Real files: {header_path} / {source_path}")

    with open(generated_definition_path, "r") as generated_fh:
        replacement = generated_fh.read()
        replace_in_file(source_path, definition_markers, replacement)

    with open(generated_implementation_path, "r") as generated_fh:
        replacement = generated_fh.read()
        replace_in_file(source_path, implementation_markers, replacement)


if __name__ == "__main__":
    main()
