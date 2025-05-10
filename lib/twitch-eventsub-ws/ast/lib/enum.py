from __future__ import annotations

from typing import List, Optional

import logging

from jinja2 import Environment

from .comment_commands import CommentCommands
from .enum_constant import EnumConstant

log = logging.getLogger(__name__)


class Enum:
    def __init__(self, name: str, namespace: tuple[str, ...]) -> None:
        self.name = name
        self.constants: List[EnumConstant] = []
        self.parent: str = ""
        self.comment_commands = CommentCommands()
        self.inner_root: str = ""
        self.namespace = namespace
        self.default: Optional[str] = None

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
        self.inner_root = comment_commands.inner_root
        self.default = comment_commands.default
