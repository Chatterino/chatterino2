#!/usr/bin/env python3

import logging
import os
import re
import sys
import argparse
from datetime import datetime
from pathlib import Path

from lib import (
    generate,
    init_clang_cindex,
    init_logging,
)

log = logging.getLogger("generate")


def _logging_level():
    def to_bool(v: str | None) -> bool:
        return bool(v) and v.lower() not in ("false", "0", "off", "no")

    return (
        logging.DEBUG
        if to_bool(os.environ.get("CI")) or to_bool(os.environ.get("DEBUG_EVENTSUB_GEN"))
        else logging.WARNING
    )


def _validate_header_path(value: str) -> str:
    if not value.endswith(".hpp"):
        raise argparse.ArgumentTypeError("Header path must end with '.hpp'")
    if not os.path.isfile(value):
        raise argparse.ArgumentTypeError(f"File '{value}' does not exist")
    return value


def main():
    init_logging(_logging_level())

    init_clang_cindex()

    if len(sys.argv) < 2:
        log.error(f"usage: {sys.argv[0]} <path-to-header>")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "header_path", type=_validate_header_path, help="Path to an existing header file ending with .hpp"
    )
    parser.add_argument(
        "--includes",
        type=lambda value: value.split(";") if value else [],
        default=[],
        help="Semicolon-separated list of include paths",
    )
    parser.add_argument("--timestamp", metavar="path", help="Path to the timestamp file to be generated")

    args = parser.parse_args()

    path = Path(args.header_path).resolve()

    if not path.is_file():
        log.error(f"{path} does not exist")
        sys.exit(1)

    if not path.name.endswith(".hpp"):
        log.error(f"{path} is not a header file")

    header_path = str(path)
    def_path = re.sub(r"\.hpp$", ".inc", header_path)
    source_path = re.sub(r"\.hpp$", ".cpp", header_path)
    source_path = re.sub(r"include[/\\]twitch-eventsub-ws[/\\]", "src/generated/", source_path)

    log.debug(f"{header_path}: definition={def_path}, implementation={source_path}")

    (definition, implementation) = generate(header_path, args.includes)

    # ensure directories are created
    Path(def_path).parent.mkdir(parents=True, exist_ok=True)
    Path(source_path).parent.mkdir(parents=True, exist_ok=True)

    with open(def_path, "w") as f:
        f.write(definition)
    with open(source_path, "w") as f:
        f.write(implementation)

    if args.timestamp:
        path = Path(args.timestamp)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(str(datetime.now().timestamp()))


if __name__ == "__main__":
    main()
