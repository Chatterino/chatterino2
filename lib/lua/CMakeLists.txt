project(lua CXX)

#[====[
This list contains all .c files except lua.c and onelua.c
Use the following command from the repository root to get these file:
perl -e 'print s/^lib\/lua\///r . "\n" for grep { /\.c$/ && !/(lua|onelua)\.c$/ } glob "lib/lua/src/*.c"'
#]====]
set(LUA_SRC 
    src/lapi.c
    src/lauxlib.c
    src/lbaselib.c
    src/lcode.c
    src/lcorolib.c
    src/lctype.c
    src/ldblib.c
    src/ldebug.c
    src/ldo.c
    src/ldump.c
    src/lfunc.c
    src/lgc.c
    src/linit.c
    src/liolib.c
    src/llex.c
    src/lmathlib.c
    src/lmem.c
    src/loadlib.c
    src/lobject.c
    src/lopcodes.c
    src/loslib.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/lstrlib.c
    src/ltable.c
    src/ltablib.c
    src/ltests.c
    src/ltm.c
    src/lundump.c
    src/lutf8lib.c
    src/lvm.c
    src/lzio.c
)

add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua
    PUBLIC
    ${LUA_INCLUDE_DIRS}
)
set_target_properties(${liblua} PROPERTIES
    LANGUAGE CXX
    LINKER_LANGUAGE CXX
    CXX_STANDARD 98
    CXX_EXTENSIONS TRUE
)
target_compile_options(lua PRIVATE
    -w # this makes clang shut up about c-as-c++
    $<$<AND:$<BOOL:${MSVC}>,$<CXX_COMPILER_ID:Clang>>:/EHsc> # enable exceptions in clang-cl
)
set_source_files_properties(${LUA_SRC} PROPERTIES LANGUAGE CXX)
