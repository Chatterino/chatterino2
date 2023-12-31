#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <QString>

#    include <vector>

struct lua_State;
namespace chatterino::lua::api {
// function names in this namespace reflect what's visible inside Lua and follow the lua naming scheme

// NOLINTBEGIN(readability-identifier-naming)
// Following functions are exposed in c2 table.

// Comments in this file are special, the docs/plugin-meta.lua file is generated from them
// All multiline comments will be added into that file. See scripts/make_luals_meta.py script for more info.

/**
 * @exposeenum c2.LogLevel
 */
// Represents "calls" to qCDebug, qCInfo ...
enum class LogLevel { Debug, Info, Warning, Critical };

/**
 * @exposeenum c2.EventType
 */
enum class EventType {
    CompletionRequested,
};

/**
 * @lua@class CommandContext
 * @lua@field words string[] The words typed when executing the command. For example `/foo bar baz` will result in `{"/foo", "bar", "baz"}`.
 * @lua@field channel_name string The name of the channel the command was executed in.
 */

/**
 * @lua@class CompletionList
 */
struct CompletionList {
    /**
     * @lua@field values string[] The completions
     */
    std::vector<QString> values{};

    /**
     * @lua@field hide_others boolean Whether other completions from Chatterino should be hidden/ignored.
     */
    bool hideOthers{};
};

/**
 * Registers a new command called `name` which when executed will call `handler`.
 *
 * @lua@param name string The name of the command.
 * @lua@param handler fun(ctx: CommandContext) The handler to be invoked when the command gets executed.
 * @lua@return boolean ok  Returns `true` if everything went ok, `false` if a command with this name exists.
 * @exposed c2.register_command
 */
int c2_register_command(lua_State *L);

/**
 * Registers a callback to be invoked when completions for a term are requested.
 *
 * @lua@param type "CompletionRequested"
 * @lua@param func fun(query: string, full_text_content: string, cursor_position: integer, is_first_word: boolean): CompletionList The callback to be invoked.
 * @exposed c2.register_callback
 */
int c2_register_callback(lua_State *L);

/**
 * Sends a message to `channel` with the specified text. Also executes commands.
 *
 * **Warning**: It is possible to trigger your own Lua command with this causing a potentially infinite loop.
 *
 * @lua@param channel string The name of the Twitch channel
 * @lua@param text string The text to be sent
 * @lua@return boolean ok
 * @exposed c2.send_msg
 */
int c2_send_msg(lua_State *L);
/**
 * Creates a system message (gray message) and adds it to the Twitch channel specified by `channel`.
 *
 * @lua@param channel string
 * @lua@param text string
 * @lua@return boolean ok
 * @exposed c2.system_msg
 */
int c2_system_msg(lua_State *L);

/**
 * Writes a message to the Chatterino log.
 *
 * @lua@param level LogLevel The desired level.
 * @lua@param ... any Values to log. Should be convertible to a string with `tostring()`.
 * @exposed c2.log
 */
int c2_log(lua_State *L);

// These ones are global
int g_load(lua_State *L);
int g_print(lua_State *L);
// NOLINTEND(readability-identifier-naming)

// This is for require() exposed as an element of package.searchers
int searcherAbsolute(lua_State *L);
int searcherRelative(lua_State *L);

}  // namespace chatterino::lua::api

#endif
