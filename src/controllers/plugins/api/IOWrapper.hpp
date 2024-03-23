#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

struct lua_State;

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)
// These functions are exposed as `_G.io`, they are wrappers for native Lua functionality.

const char *const REG_REAL_IO_NAME = "real_lua_io_lib";
const char *const REG_C2_IO_NAME = "c2io";

/**
 * Opens a file.
 * If given a relative path, it will be relative to
 * c2datadir/Plugins/pluginDir/data/
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.open
 *
 * @lua@param filename string
 * @lua@param mode nil|"r"|"w"|"a"|"r+"|"w+"|"a+"
 * @exposed io.open
 */
int io_open(lua_State *L);

/**
 * Equivalent to io.input():lines("l") or a specific iterator over given file
 * If given a relative path, it will be relative to
 * c2datadir/Plugins/pluginDir/data/
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.lines
 *
 * @lua@param filename nil|string
 * @lua@param ...
 * @exposed io.lines
 */
int io_lines(lua_State *L);

/**
 * Opens a file and sets it as default input or if given no arguments returns the default input.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.input
 *
 * @lua@param fileorname nil|string|FILE*
 * @lua@return nil|FILE*
 * @exposed io.input
 */
int io_input(lua_State *L);

/**
 * Opens a file and sets it as default output or if given no arguments returns the default output
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.output
 *
 * @lua@param fileorname nil|string|FILE*
 * @lua@return nil|FILE*
 * @exposed io.output
 */
int io_output(lua_State *L);

/**
 * Closes given file or io.output() if not given.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.close
 *
 * @lua@param nil|FILE*
 * @exposed io.close
 */
int io_close(lua_State *L);

/**
 * Flushes the buffer for given file or io.output() if not given.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.flush
 *
 * @lua@param nil|FILE*
 * @exposed io.flush
 */
int io_flush(lua_State *L);

/**
 * Reads some data from the default input file
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.read
 *
 * @lua@param nil|string
 * @exposed io.read
 */
int io_read(lua_State *L);

/**
 * Writes some data to the default output file
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.write
 *
 * @lua@param nil|string
 * @exposed io.write
 */
int io_write(lua_State *L);

int io_popen(lua_State *L);
int io_tmpfile(lua_State *L);

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
#endif
