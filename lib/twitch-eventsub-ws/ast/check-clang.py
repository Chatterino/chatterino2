import clang.cindex

from lib import init_clang_cindex

init_clang_cindex()
clang.cindex.Index.create()
