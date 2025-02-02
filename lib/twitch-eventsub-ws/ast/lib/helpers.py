from typing import Generator, List, Tuple

import contextlib
import logging
import os
import subprocess
from io import TextIOWrapper
from tempfile import mkstemp

import clang.cindex

log = logging.getLogger(__name__)


def init_clang_cindex() -> None:
    # 1. Has the LIBCLANG_LIBRARY_FILE environment variable been set? Use it
    env_library_file = os.environ.get("LIBCLANG_LIBRARY_FILE")
    if env_library_file is not None:
        log.debug(f"Setting clang library file from LIBCLANG_LIBRARY_PATH environment variable: {env_library_file}")
        clang.cindex.Config.set_library_file(env_library_file)
        return

    # 2. Has the LIBCLANG_LIBRARY_PATH environment variable been set? Use it
    env_library_path = os.environ.get("LIBCLANG_LIBRARY_PATH")
    if env_library_path is not None:
        log.debug(
            f"Setting clang library search path from LIBCLANG_LIBRARY_PATH environment variable: {env_library_path}"
        )
        clang.cindex.Config.set_library_path(env_library_path)
        return

    import ctypes.util

    autodetected_path = ctypes.util.find_library("libclang" if os.name == "nt" else "clang")
    if autodetected_path is not None:
        log.debug(f"Autodetected clang library file: {autodetected_path}")
        clang.cindex.Config.set_library_file(autodetected_path)
        return


@contextlib.contextmanager
def temporary_file() -> Generator[Tuple[TextIOWrapper, str], None, None]:
    fd, path = mkstemp()
    log.debug(f"Opening file {path}")
    f = os.fdopen(fd, "w")

    yield (f, path)

    f.flush()
    f.close()
    log.debug(f"Closed file {path}")


def get_clang_builtin_include_dirs() -> Tuple[List[str], List[str]]:
    quote_includes: List[str] = []
    angle_includes: List[str] = []

    if os.name == "nt":
        return [], []

    cmd = [
        "clang++",
        "-E",
        "-x",
        "c++",
        "-v",
        "-",
    ]

    path_entries = os.environ.get("PATH", "").split(os.pathsep)

    llvm_path = os.environ.get("LLVM_PATH")
    if llvm_path is not None:
        path_entries.insert(0, os.path.join(llvm_path, "bin"))

    path_str = os.pathsep.join(path_entries)

    full_env = os.environ
    full_env["PATH"] = path_str

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        env=full_env,
    )

    _, errs = proc.communicate(input=None, timeout=10)

    doing_quote_includes = False
    doing_angle_includes = False

    for line in errs.decode("utf8").splitlines():
        line = line.strip()
        if line == r'#include "..." search starts here:':
            doing_angle_includes = False
            doing_quote_includes = True
            continue
        if line == r"#include <...> search starts here:":
            doing_quote_includes = False
            doing_angle_includes = True
            continue
        if line == r"End of search list.":
            doing_quote_includes = False
            doing_angle_includes = False
            continue

        if doing_quote_includes or doing_angle_includes:
            if " " in line:
                continue
            line = line.split(" ", 1)[0]
            p = os.path.realpath(line)
            if doing_quote_includes:
                quote_includes.append(p)
            if doing_angle_includes:
                angle_includes.append(p)

    return (quote_includes, angle_includes)
