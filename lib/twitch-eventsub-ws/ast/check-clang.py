import clang.cindex
import sys
from pathlib import Path

from lib import init_clang_cindex, parse

init_clang_cindex()

assert len(sys.argv) == 2
additional_includes = sys.argv[1].split(";")
print("includes:", additional_includes)

tu = parse(Path(__file__).parent / "check-clang.hpp", additional_includes)

if len(tu.diagnostics) > 0:
    for diag in tu.diagnostics:
        print(diag.location)
        print(diag.spelling)
        print(diag.option)
    assert False, "TU had warnings/errors when parsing"
