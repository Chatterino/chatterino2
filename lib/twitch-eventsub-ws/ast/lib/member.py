from __future__ import annotations

from typing import Optional, List

import logging

import clang.cindex
from clang.cindex import CursorKind

from .comment_commands import CommentCommands, json_transform, parse_comment_commands
from .membertype import MemberType

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


class Member:
    def __init__(
        self,
        name: str,
        member_type: MemberType = MemberType.BASIC,
        type_name: str = "?",
    ) -> None:
        self.name = name
        self.json_name = name
        self.member_type = member_type
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
    def from_field(node: clang.cindex.Cursor, namespace: List[str]) -> Member:
        assert node.type is not None

        name = node.spelling
        member_type = MemberType.BASIC
        type_name = get_type_name(node.type, namespace)

        log.debug(f"{node.spelling} - {type_name} - {node.type.is_const_qualified()}")

        ntargs = node.type.get_num_template_arguments()
        if ntargs > 0:
            overwrite_member_type: Optional[MemberType] = None

            # log.debug(node.type.get_template_argument_type(0).kind)
            # log.debug(node.type.get_template_argument_type(0).spelling)
            # log.debug(node.type.get_template_argument_type(0).get_named_type().spelling)
            # log.debug(node.type.get_template_argument_type(0).get_class_type().spelling)

            type_name = get_type_name(node.type.get_template_argument_type(0), namespace)

            for xd in node.get_children():
                match xd.kind:
                    case CursorKind.NAMESPACE_REF:
                        # Ignore namespaces
                        pass

                    case CursorKind.TEMPLATE_REF:
                        match xd.spelling:
                            case "optional":
                                match overwrite_member_type:
                                    case None:
                                        overwrite_member_type = MemberType.OPTIONAL
                                    case other:
                                        log.warning(f"Optional cannot be added on top of other member type: {other}")

                            case "vector":
                                match overwrite_member_type:
                                    case None:
                                        overwrite_member_type = MemberType.VECTOR
                                    case MemberType.OPTIONAL:
                                        overwrite_member_type = MemberType.OPTIONAL_VECTOR
                                    case other:
                                        log.warning(f"Vector cannot be added on top of other member type: {other}")

                            case other:
                                log.warning(f"Unhandled template type: {other}")

                    case CursorKind.TYPE_REF:
                        type_name = get_type_name(xd.type, namespace)

                    case other:
                        log.debug(f"Unhandled child kind type: {other}")

                if overwrite_member_type is not None:
                    member_type = overwrite_member_type

        member = Member(name, member_type, type_name)

        if node.raw_comment is not None:
            comment_commands = parse_comment_commands(node.raw_comment)
            member.apply_comment_commands(comment_commands)

        return member

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, self.__class__):
            return False

        if self.name != other.name:
            return False
        if self.member_type != other.member_type:
            return False
        if self.type_name != other.type_name:
            return False

        return True

    def __repr__(self) -> str:
        match self.member_type:
            case MemberType.BASIC:
                return f"{self.type_name} {self.name}"

            case MemberType.VECTOR:
                return f"std::vector<{self.type_name}> {self.name}"

            case MemberType.OPTIONAL:
                return f"std::optional<{self.type_name}> {self.name}"

            case MemberType.OPTIONAL_VECTOR:
                return f"std::optional<std::vector<{self.type_name}>> {self.name}"
