#!/usr/bin/env python3

import logging

from lib import get_clang_builtin_include_dirs, init_logging

log = logging.getLogger(__name__)


def main():
    init_logging()

    quote_includes, angle_includes = get_clang_builtin_include_dirs()

    print("Quote includes:")
    print("\n".join(quote_includes))
    print()

    print("Angle includes:")
    print("\n".join(angle_includes))
    print()


if __name__ == "__main__":
    main()
