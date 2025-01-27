from __future__ import annotations

from typing import List

import logging

from jinja2 import Environment

from .comment_commands import CommentCommands
from .member import Member

log = logging.getLogger(__name__)


class Struct:
    def __init__(self, name: str) -> None:
        self.name = name
        self.members: List[Member] = []
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

            return self.members == other.members
        return False

    def __str__(self) -> str:
        pretty_members = "\n  ".join(map(str, self.members))
        return f"struct {self.name} {{\n  {pretty_members}\n}}"

    def try_value_to_implementation(self, env: Environment) -> str:
        return env.get_template("struct-implementation.tmpl").render(struct=self)

    def try_value_to_definition(self, env: Environment) -> str:
        return env.get_template("struct-definition.tmpl").render(struct=self)

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        for command, value in comment_commands:
            match command:
                case "json_rename":
                    # Do nothing on structs
                    pass
                case "json_dont_fail_on_deserialization":
                    # Do nothing on structs
                    pass
                case "json_transform":
                    # Do nothing on structs
                    pass
                case "json_inner":
                    self.inner_root = value
                case other:
                    log.warning(f"Unknown comment command found: {other} with value {value}")
