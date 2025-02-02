import logging
import subprocess

log = logging.getLogger(__name__)


def format_code(code: str) -> str:
    proc = subprocess.Popen(["clang-format", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    outs, errs = proc.communicate(input=code.encode(), timeout=2)
    if errs is not None:
        log.warning(f"Error formatting code: {errs.decode()}")

    return outs.decode()
