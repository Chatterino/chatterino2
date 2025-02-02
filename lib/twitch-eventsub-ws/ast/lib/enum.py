from __future__ import annotations

from typing import List

import logging

from jinja2 import Environment

from .comment_commands import CommentCommands
from .enum_constant import EnumConstant

log = logging.getLogger(__name__)


class Enum:
    def __init__(self, name: str) -> None:
        self.name = name
        self.constants: List[EnumConstant] = []
        self.parent: str = ""
        self.comment_commands: CommentCommands = []
        self.inner_root: str = ""

    @property
    def full_name(self) -> str:
        if self.parent:
            return f"{self.parent}::{self.name}"
        else:
            return self.name

    def __eq__(self, other: object) -> bool:
        if isinstance(other, self.__class__):
            if self.name != other.name:
                return False

            return self.constants == other.constants
        return False

    def __str__(self) -> str:
        pretty_constants = "\n  ".join(map(str, self.constants))
        return f"enum class {self.name} {{\n  {pretty_constants}\n}}"

    def try_value_to_implementation(self, env: Environment) -> str:
        return env.get_template("enum-implementation.tmpl").render(enum=self)

    def try_value_to_definition(self, env: Environment) -> str:
        return env.get_template("enum-definition.tmpl").render(enum=self)

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        for command, value in comment_commands:
            match command:
                case "json_rename":
                    # Do nothing on enums
                    pass
                case "json_dont_fail_on_deserialization":
                    # Do nothing on enums
                    pass
                case "json_transform":
                    # Do nothing on enums
                    pass
                case "json_inner":
                    self.inner_root = value
                case other:
                    log.warning(f"Unknown comment command found: {other} with value {value}")
