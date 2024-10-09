#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include <QString>
#    include <sol/types.hpp>
#    include <sol/variadic_args.hpp>
#    include <sol/variadic_results.hpp>

struct lua_State;

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)
// These functions are exposed as `_G.io`, they are wrappers for native Lua functionality.

const char *const REG_REAL_IO_NAME = "real_lua_io_lib";

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
sol::variadic_results io_open(sol::this_state L, QString filename,
                              QString strmode);
sol::variadic_results io_open_modeless(sol::this_state L, QString filename);

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
sol::variadic_results io_lines(sol::this_state L, QString filename,
                               sol::variadic_args args);
sol::variadic_results io_lines_noargs(sol::this_state L);

/**
 * Opens a file and sets it as default input or if given no arguments returns the default input.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.input
 *
 * @lua@param fileorname nil|string|FILE*
 * @lua@return nil|FILE*
 * @exposed io.input
 */
sol::variadic_results io_input_argless(sol::this_state L);
sol::variadic_results io_input_file(sol::this_state L, sol::userdata file);
sol::variadic_results io_input_name(sol::this_state L, QString filename);

/**
 * Opens a file and sets it as default output or if given no arguments returns the default output
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.output
 *
 * @lua@param fileorname nil|string|FILE*
 * @lua@return nil|FILE*
 * @exposed io.output
 */
sol::variadic_results io_output_argless(sol::this_state L);
sol::variadic_results io_output_file(sol::this_state L, sol::userdata file);
sol::variadic_results io_output_name(sol::this_state L, QString filename);

/**
 * Closes given file or io.output() if not given.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.close
 *
 * @lua@param nil|FILE*
 * @exposed io.close
 */
bool io_close_argless(sol::this_state L);
bool io_close_file(sol::this_state L, sol::userdata file);

/**
 * Flushes the buffer for given file or io.output() if not given.
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.flush
 *
 * @lua@param nil|FILE*
 * @exposed io.flush
 */
void io_flush_argless(sol::this_state L);
void io_flush_file(sol::this_state L, sol::userdata file);

/**
 * Reads some data from the default input file
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.read
 *
 * @lua@param nil|string
 * @exposed io.read
 */
sol::variadic_results io_read(sol::this_state L, sol::variadic_args args);

/**
 * Writes some data to the default output file
 * See https://www.lua.org/manual/5.4/manual.html#pdf-io.write
 *
 * @lua@param nil|string
 * @exposed io.write
 */
sol::variadic_results io_write(sol::this_state L, sol::variadic_args args);

void io_popen();
void io_tmpfile();

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
#endif
