project(lua CXX)

#[====[
Updating this list:
remove all listed files
go to line below, ^y2j4j$@" and then reindent the file names
/LUA_SRC
:r!ls lib/lua/src | grep '\.c' | grep -Ev 'lua\.c|onelua\.c' | sed 's#^#src/#'

#]====]
set(LUA_SRC 
    "src/lapi.c"
    "src/lauxlib.c"
    "src/lbaselib.c"
    "src/lcode.c"
    "src/lcorolib.c"
    "src/lctype.c"
    "src/ldblib.c"
    "src/ldebug.c"
    "src/ldo.c"
    "src/ldump.c"
    "src/lfunc.c"
    "src/lgc.c"
    "src/linit.c"
    "src/liolib.c"
    "src/llex.c"
    "src/lmathlib.c"
    "src/lmem.c"
    "src/loadlib.c"
    "src/lobject.c"
    "src/lopcodes.c"
    "src/loslib.c"
    "src/lparser.c"
    "src/lstate.c"
    "src/lstring.c"
    "src/lstrlib.c"
    "src/ltable.c"
    "src/ltablib.c"
    "src/ltests.c"
    "src/ltm.c"
    "src/lua.c"
    "src/lundump.c"
    "src/lutf8lib.c"
    "src/lvm.c"
    "src/lzio.c"
)

add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua
    PUBLIC
    ${LUA_INCLUDE_DIRS}
)
set_source_files_properties(${LUA_SRC} PROPERTIES LANGUAGE C)
