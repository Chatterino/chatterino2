from __future__ import annotations

from typing import Optional, List

import logging

import clang.cindex
from clang.cindex import CursorKind

from .comment_commands import CommentCommands, json_transform, parse_comment_commands

log = logging.getLogger(__name__)


def get_type_name(type: clang.cindex.Type, namespace: List[str]) -> str:
    if namespace:
        namespace_str = f"{'::'.join(namespace)}::"
    else:
        namespace_str = ""
    type_name = type.spelling

    if type.is_const_qualified():
        type_name = type_name.replace("const", "").strip()

    type_name = type_name.removeprefix(namespace_str)

    return type_name


class EnumConstant:
    def __init__(
        self,
        name: str,
        type_name: str = "?",
    ) -> None:
        self.name = name
        self.json_name = name
        self.type_name = type_name
        self.tag: Optional[str] = None

        self.dont_fail_on_deserialization: bool = False

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        for command, value in comment_commands:
            match command:
                case "json_rename":
                    # Rename the key that this field will use in json terms
                    log.debug(f"Rename json key from {self.json_name} to {value}")
                    self.json_name = value
                case "json_dont_fail_on_deserialization":
                    # Don't fail when an optional object exists and its data is bad
                    log.debug(f"Don't fail on deserialization for {self.name}")
                    self.dont_fail_on_deserialization = bool(value.lower() == "true")
                case "json_transform":
                    # Transform the key from whatever-case to case specified by `value`
                    self.json_name = json_transform(self.json_name, value)
                case "json_inner":
                    # Do nothing on members
                    pass
                case "json_tag":
                    # Rename the key that this field will use in json terms
                    log.debug(f"Applied json tag on {self.json_name}: {value}")
                    self.tag = value
                case other:
                    log.warning(f"Unknown comment command found: {other} with value {value}")

    @staticmethod
    def from_field(node: clang.cindex.Cursor, namespace: List[str]) -> EnumConstant:
        assert node.type is not None

        name = node.spelling
        type_name = get_type_name(node.type, namespace)

        log.warning(f"{node.spelling} - {type_name} - {node.type.is_const_qualified()}")

        ntargs = node.type.get_num_template_arguments()
        if ntargs > 0:
            type_name = get_type_name(node.type.get_template_argument_type(0), namespace)

            for xd in node.get_children():
                match xd.kind:
                    case CursorKind.NAMESPACE_REF:
                        # Ignore namespaces
                        pass

                    case other:
                        log.warning(f"Unhandled child kind type: {other}")

        enum = EnumConstant(name, type_name)

        if node.raw_comment is not None:
            comment_commands = parse_comment_commands(node.raw_comment)
            enum.apply_comment_commands(comment_commands)

        return enum

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, self.__class__):
            return False

        if self.name != other.name:
            return False
        if self.type_name != other.type_name:
            return False

        return True

    def __repr__(self) -> str:
        return f"{self.name}"
