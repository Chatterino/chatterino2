from __future__ import annotations

from typing import Optional
from dataclasses import dataclass

import logging

import clang.cindex
from clang.cindex import CursorKind

from .comment_commands import CommentCommands
from .membertype import MemberType

log = logging.getLogger(__name__)


def get_type_name(type: clang.cindex.Type, namespace: tuple[str, ...]) -> str:
    if namespace:
        namespace_str = f"{'::'.join(namespace)}::"
    else:
        namespace_str = ""
    type_name = type.spelling

    if type.is_const_qualified():
        type_name = type_name.replace("const", "").strip()

    type_name = type_name.removeprefix(namespace_str)

    return type_name


def _get_template_name(type: clang.cindex.Type) -> str:
    type = type.get_canonical()
    if type.get_num_template_arguments() < 1:
        return type.spelling
    name: str = type.spelling
    if type.is_const_qualified():
        name.removeprefix("const ")
    return name[: name.index("<")]


def _is_chrono_like_type(type: clang.cindex.Type) -> bool:
    return _get_template_name(type) in ("std::chrono::time_point", "std::chrono::duration")


# clang's C API doesn't expose this, so we emulate it
def _is_trivially_copyable(type: clang.cindex.Type) -> bool:
    # remove optional wrapper(s)
    type = type.get_canonical()
    while type.get_num_template_arguments() and _get_template_name(type) == "std::optional":
        type = type.get_template_argument_type(0).get_canonical()

    if type.is_pod():
        return True
    return _is_chrono_like_type(type)


@dataclass
class VariantType:
    name: str
    trivial: bool
    empty: bool

    @property
    def id(self) -> str:
        return self.name.replace(":", "")


class Member:
    def __init__(
        self,
        name: str,
        member_type: MemberType = MemberType.BASIC,
        type_name: str = "?",
        trivial: bool = False,
    ) -> None:
        self.name = name
        self.json_name = name
        self.member_type = member_type
        self.type_name = type_name
        self.tag: Optional[str] = None
        self.trivial = trivial
        self.variant_types: list[VariantType] | None = None
        self.variant_fallback: str | None = None

        self.dont_fail_on_deserialization: bool = False

        self._infer_tags()

    def _infer_tags(self) -> None:
        match self.type_name:
            case "std::chrono::system_clock::time_point":
                assert self.tag is None
                self.tag = "AsISO8601"

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        self.json_name = comment_commands.apply_name_transform(self.json_name)
        if comment_commands.tag is not None:
            self.tag = comment_commands.tag
        self.dont_fail_on_deserialization = comment_commands.dont_fail_on_deserialization

    @staticmethod
    def from_field(
        node: clang.cindex.Cursor,
        comment_commands: CommentCommands,
        namespace: tuple[str, ...],
    ) -> Member:
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
                            case "variant":
                                overwrite_member_type = MemberType.VARIANT
                            case other:
                                log.warning(f"Unhandled template type: {other}")

                    case CursorKind.TYPE_REF:
                        type_name = get_type_name(xd.type, namespace)

                    case other:
                        log.debug(f"Unhandled child kind type: {other}")

                if overwrite_member_type is not None:
                    member_type = overwrite_member_type

        member = Member(name, member_type, type_name, _is_trivially_copyable(node.type))

        if node.raw_comment is not None:
            comment_commands.parse(node.raw_comment)

        member.apply_comment_commands(comment_commands)

        if member.member_type == MemberType.VARIANT:
            member.apply_variant(node.type, namespace)

        return member

    def apply_variant(self, type: clang.cindex.Type, namespace: tuple[str, ...]):
        self.variant_types = []
        for idx in range(type.get_num_template_arguments()):
            inner = type.get_template_argument_type(idx)
            name = get_type_name(inner, namespace)
            if name == "std::string" or name == "String":
                assert not self.variant_fallback
                self.variant_fallback = name
                continue
            self.variant_types.append(
                VariantType(
                    name=name,
                    trivial=_is_trivially_copyable(inner),
                    empty=inner.get_size() <= 1,
                )
            )

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
