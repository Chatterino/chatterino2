from typing import List, Optional

import logging
import os

import clang.cindex
from clang.cindex import CursorKind

from .comment_commands import parse_comment_commands
from .member import Member
from .enum_constant import EnumConstant
from .struct import Struct
from .enum import Enum

log = logging.getLogger(__name__)


class Walker:
    def __init__(self, filename: str) -> None:
        self.filename = filename
        self.real_filepath = os.path.realpath(self.filename)
        self.structs: List[Struct] = []
        self.enums: List[Enum] = []
        self.namespace: List[str] = []

    def handle_node(self, node: clang.cindex.Cursor, struct: Optional[Struct], enum: Optional[Enum]) -> bool:
        match node.kind:
            case CursorKind.STRUCT_DECL:
                new_struct = Struct(node.spelling)
                if node.raw_comment is not None:
                    new_struct.comment_commands = parse_comment_commands(node.raw_comment)
                    new_struct.apply_comment_commands(new_struct.comment_commands)
                if struct is not None:
                    new_struct.parent = struct.full_name

                for child in node.get_children():
                    self.handle_node(child, new_struct, None)

                self.structs.append(new_struct)

                return True

            case CursorKind.ENUM_DECL:
                new_enum = Enum(node.spelling)
                if node.raw_comment is not None:
                    new_enum.comment_commands = parse_comment_commands(node.raw_comment)
                    new_enum.apply_comment_commands(new_enum.comment_commands)
                if struct is not None:
                    new_enum.parent = struct.full_name

                for child in node.get_children():
                    self.handle_node(child, None, new_enum)

                self.enums.append(new_enum)

                return True

            case CursorKind.ENUM_CONSTANT_DECL:
                if enum:
                    # log.warning(
                    #     f"enum constant decl {node.spelling} - enum comments: {enum.comment_commands} - node comments: {node.raw_comment}"
                    # )
                    constant = EnumConstant.from_node(node)
                    constant.apply_comment_commands(enum.comment_commands)
                    enum.constants.append(constant)

            case CursorKind.FIELD_DECL:
                type = node.type
                if type is None:
                    # Skip nodes without a type
                    return False

                # log.debug(f"{struct}: {type.spelling} {node.spelling} ({type.kind})")
                if struct:
                    member = Member.from_field(node, self.namespace)
                    member.apply_comment_commands(struct.comment_commands)
                    struct.members.append(member)

            case CursorKind.NAMESPACE:
                self.namespace.append(node.spelling)
                # Ignore namespaces
                pass

            case CursorKind.FUNCTION_DECL:
                # Ignore function declarations
                pass

            case _:
                # log.warning(f"unhandled kind: {node.kind}")
                pass

        return False

    def walk(self, node: clang.cindex.Cursor) -> None:
        if node.location.file and not clang.cindex.Config().lib.clang_Location_isFromMainFile(node.location):
            return

        handled = self.handle_node(node, None, None)

        if not handled:
            for child in node.get_children():
                self.walk(child)
