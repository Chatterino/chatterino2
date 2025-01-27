from lib.build import build_structs
from lib.helpers import init_clang_cindex
from lib.membertype import MemberType

import pytest


def init_clang():
    init_clang_cindex()


def test_simple():
    structs = build_structs("lib/tests/resources/simple.hpp")

    assert len(structs) == 1

    s = structs[0]

    assert s.name == "Simple"
    assert len(s.members) == 3

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.BASIC
    assert s.members[0].type_name == "int"

    assert s.members[1].name == "b"
    assert s.members[1].member_type == MemberType.BASIC
    assert s.members[1].type_name == "bool"

    assert s.members[2].name == "c"
    assert s.members[2].member_type == MemberType.BASIC
    assert s.members[2].type_name == "char"


def test_vector():
    import clang.cindex

    print(clang.cindex.conf.get_filename())
    structs = build_structs("lib/tests/resources/vector.hpp")
    assert len(structs) == 1
    s = structs[0]

    assert s.name == "Vector"
    assert len(s.members) == 2

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.VECTOR
    assert s.members[0].type_name == "bool"

    assert s.members[1].name == "b"
    assert s.members[1].member_type == MemberType.VECTOR
    assert s.members[1].type_name == "std::vector<bool>"


def test_optional():
    import clang.cindex

    print(clang.cindex.conf.get_filename())
    structs = build_structs("lib/tests/resources/optional.hpp")
    assert len(structs) == 1
    s = structs[0]

    assert s.name == "Optional"
    assert len(s.members) == 1

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.OPTIONAL
    assert s.members[0].type_name == "bool"


def test_vector_pod():
    import clang.cindex

    print(clang.cindex.conf.get_filename())
    structs = build_structs("lib/tests/resources/vector-pod.hpp")
    assert len(structs) == 2

    s = structs[0]

    assert s.name == "Pod"
    assert len(s.members) == 3

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.BASIC
    assert s.members[0].type_name == "int"

    assert s.members[1].name == "b"
    assert s.members[1].member_type == MemberType.BASIC
    assert s.members[1].type_name == "bool"

    assert s.members[2].name == "c"
    assert s.members[2].member_type == MemberType.BASIC
    assert s.members[2].type_name == "char"

    s = structs[1]

    assert s.name == "VectorPod"
    assert len(s.members) == 1

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.VECTOR
    assert s.members[0].type_name == "Pod"


def test_const():
    import clang.cindex

    print(clang.cindex.conf.get_filename())
    structs = build_structs("lib/tests/resources/const.hpp")
    assert len(structs) == 2

    s = structs[0]
    assert s.name == "Pod"
    assert len(s.members) == 0

    s = structs[1]

    assert s.name == "Const"
    assert len(s.members) == 6

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.BASIC
    assert s.members[0].type_name == "int"

    assert s.members[1].name == "b"
    assert s.members[1].member_type == MemberType.BASIC
    assert s.members[1].type_name == "bool"

    assert s.members[2].name == "c"
    assert s.members[2].member_type == MemberType.BASIC
    assert s.members[2].type_name == "char"

    assert s.members[3].name == "d"
    assert s.members[3].member_type == MemberType.BASIC
    assert s.members[3].type_name == "Pod"

    assert s.members[4].name == "e"
    assert s.members[4].member_type == MemberType.VECTOR
    assert s.members[4].type_name == "bool"

    assert s.members[5].name == "f"
    assert s.members[5].member_type == MemberType.OPTIONAL
    assert s.members[5].type_name == "bool"


def test_string():
    import clang.cindex

    print(clang.cindex.conf.get_filename())
    structs = build_structs("lib/tests/resources/string.hpp")
    assert len(structs) == 1

    s = structs[0]

    assert s.name == "String"
    assert len(s.members) == 4

    assert s.members[0].name == "a"
    assert s.members[0].member_type == MemberType.BASIC
    assert s.members[0].type_name == "std::string"

    assert s.members[1].name == "b"
    assert s.members[1].member_type == MemberType.BASIC
    assert s.members[1].type_name == "std::string"

    assert s.members[2].name == "c"
    assert s.members[2].member_type == MemberType.VECTOR
    assert s.members[2].type_name == "std::string"

    assert s.members[3].name == "d"
    assert s.members[3].member_type == MemberType.OPTIONAL
    assert s.members[3].type_name == "std::string"


init_clang()
