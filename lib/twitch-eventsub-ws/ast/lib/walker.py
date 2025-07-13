from typing import List, Optional

import logging
import os

import clang.cindex
from clang.cindex import CursorKind

from .comment_commands import CommentCommands
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
        self.namespace: tuple[str, ...] = ()

    def handle_node(self, node: clang.cindex.Cursor, struct: Optional[Struct], enum: Optional[Enum]) -> bool:
        match node.kind:
            case CursorKind.STRUCT_DECL:
                new_struct = Struct(node.spelling, self.namespace)
                if node.raw_comment is not None:
                    new_struct.comment_commands.parse(node.raw_comment)
                    new_struct.apply_comment_commands(new_struct.comment_commands)
                if struct is not None:
                    new_struct.parent = struct.full_name

                for child in node.get_children():
                    self.handle_node(child, new_struct, None)

                new_struct.validate()
                self.structs.append(new_struct)

                return True

            case CursorKind.ENUM_DECL:
                new_enum = Enum(node.spelling, self.namespace)
                if node.raw_comment is not None:
                    new_enum.comment_commands.parse(node.raw_comment)
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
                    constant = EnumConstant.from_node(node, CommentCommands(enum.comment_commands))
                    enum.constants.append(constant)

            case CursorKind.FIELD_DECL:
                type = node.type
                if type is None:
                    # Skip nodes without a type
                    return False
                if node.storage_class == clang.cindex.StorageClass.STATIC:
                    return False

                # log.debug(f"{struct}: {type.spelling} {node.spelling} ({type.kind})")
                if struct:
                    member = Member.from_field(node, CommentCommands(struct.comment_commands), self.namespace)
                    struct.members.append(member)

            case CursorKind.NAMESPACE:
                self.namespace += (node.spelling,)
                for child in node.get_children():
                    self.walk(child)
                self.namespace = self.namespace[:-1]
                return True

            case CursorKind.CXX_BASE_SPECIFIER:
                assert struct
                struct.base = node.type.spelling
                return False

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
