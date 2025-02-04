import os

import clang.cindex
from .helpers import get_clang_builtin_include_dirs


def parse(filename: str, additional_includes: list[str] = []) -> clang.cindex.TranslationUnit:
    if not os.path.isfile(filename):
        raise ValueError(f"Path {filename} is not a file. cwd: {os.getcwd()}")

    parse_args = [
        "-std=c++17",
        # Uncomment this if you need to debug where it tries to find headers
        # "-H",
    ]

    parse_options = (
        clang.cindex.TranslationUnit.PARSE_INCOMPLETE | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
    )

    extra_includes, system_includes = get_clang_builtin_include_dirs()

    for include_dir in system_includes:
        parse_args.append(f"-isystem{include_dir}")

    for include_dir in extra_includes:
        parse_args.append(f"-I{include_dir}")

    for dir in additional_includes:
        parse_args.append("-I")
        parse_args.append(dir)

    # Append default dirs
    # - Append dir of file
    file_dir = os.path.dirname(os.path.realpath(filename))
    parse_args.append(f"-I{file_dir}")
    # - Append project include dir
    file_subdir = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(filename)), "../.."))
    parse_args.append(f"-I{file_subdir}")

    # TODO: Use build_commands if available

    index = clang.cindex.Index.create()

    return index.parse(filename, args=parse_args, options=parse_options)
