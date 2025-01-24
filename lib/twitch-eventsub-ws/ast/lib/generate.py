from typing import Tuple

import logging

from .build import build_structs
from .format import format_code
from .helpers import init_clang_cindex, temporary_file
from .jinja_env import env
from .logging import init_logging

log = logging.getLogger(__name__)


def generate(header_path: str, additional_includes: list[str] = []) -> Tuple[str, str]:
    structs = build_structs(header_path, additional_includes)

    log.debug("Generate & format definitions")
    definitions = format_code("\n\n".join([struct.try_value_to_definition(env) for struct in structs]))
    log.debug("Generate & format implementations")
    implementations = format_code("\n\n".join([struct.try_value_to_implementation(env) for struct in structs]))

    return (definitions, implementations)
