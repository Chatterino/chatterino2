#!/usr/bin/env python3

import logging
import sys

from lib import generate, init_clang_cindex, init_logging, temporary_file

log = logging.getLogger("generate")


def main():
    init_logging()

    init_clang_cindex()

    if len(sys.argv) < 2:
        log.error(f"Missing header file argument. Usage: {sys.argv[0]} <path-to-header-file>")
        sys.exit(1)

    (definitions, implementations) = generate(sys.argv[1])

    with temporary_file() as (f, path):
        log.debug(f"Write definitions to {path}")
        f.write(definitions)
        f.write("\n")
        print(path)

    with temporary_file() as (f, path):
        log.debug(f"Write implementations to {path}")
        f.write(implementations)
        f.write("\n")
        print(path)


if __name__ == "__main__":
    main()
