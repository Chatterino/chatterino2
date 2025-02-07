from __future__ import annotations

from typing import Optional

import logging

import clang.cindex

from .comment_commands import CommentCommands

log = logging.getLogger(__name__)


class EnumConstant:
    def __init__(
        self,
        name: str,
    ) -> None:
        self.name = name
        self.json_name = name
        self.tag: Optional[str] = None

        self.dont_fail_on_deserialization: bool = False

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        self.json_name = comment_commands.apply_name_transform(self.json_name)
        self.tag = comment_commands.tag
        self.dont_fail_on_deserialization = comment_commands.dont_fail_on_deserialization

    @staticmethod
    def from_node(
        node: clang.cindex.Cursor,
        comment_commands: CommentCommands,
    ) -> EnumConstant:
        assert node.type is not None

        name = node.spelling

        enum = EnumConstant(name)

        if node.raw_comment is not None:
            comment_commands.parse(node.raw_comment)

        enum.apply_comment_commands(comment_commands)

        return enum

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, self.__class__):
            return False

        if self.name != other.name:
            return False

        return True

    def __repr__(self) -> str:
        return f"{self.name}"
