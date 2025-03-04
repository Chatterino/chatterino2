from __future__ import annotations

from typing import List, Optional

import logging

from jinja2 import Environment

from .comment_commands import CommentCommands
from .member import Member

log = logging.getLogger(__name__)


class Struct:
    def __init__(self, name: str, namespace: tuple[str, ...]) -> None:
        self.name = name
        self.members: List[Member] = []
        self.parent: str = ""
        self.comment_commands = CommentCommands()
        self.inner_root: str = ""
        self.namespace = namespace
        self.base: Optional[str] = None

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
        self.inner_root = comment_commands.inner_root

    def validate(self) -> None:
        assert not (self.base and self.members), (
            f"Unsupported: Struct {self.name} has both a base class as well as additional members"
        )
