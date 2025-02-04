from typing import List

import logging
import os

import clang.cindex

from .helpers import get_clang_builtin_include_dirs
from .struct import Struct
from .enum import Enum
from .walker import Walker
from .parse import parse

log = logging.getLogger(__name__)


def build_structs(filename: str, additional_includes: list[str] = []) -> List[Struct | Enum]:
    log.debug("Parsing translation unit")
    tu = parse(filename, additional_includes)
    root = tu.cursor

    for diag in tu.diagnostics:
        log.warning(diag.location)
        log.warning(diag.spelling)
        log.warning(diag.option)

    log.debug("Walking over AST")
    walker = Walker(filename)
    walker.walk(root)

    return walker.enums + walker.structs
