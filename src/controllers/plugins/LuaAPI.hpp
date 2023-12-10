#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <QString>

#    include <vector>

struct lua_State;
namespace chatterino::lua::api {
// function names in this namespace reflect what's visible inside Lua and follow the lua naming scheme

// NOLINTBEGIN(readability-identifier-naming)
// Following functions are exposed in c2 table.
int c2_register_command(lua_State *L);
int c2_register_callback(lua_State *L);
int c2_send_msg(lua_State *L);
int c2_system_msg(lua_State *L);
int c2_log(lua_State *L);

// These ones are global
int g_load(lua_State *L);
int g_print(lua_State *L);
// NOLINTEND(readability-identifier-naming)

// This is for require() exposed as an element of package.searchers
int searcherAbsolute(lua_State *L);
int searcherRelative(lua_State *L);

// Exposed as c2.LogLevel
// Represents "calls" to qCDebug, qCInfo ...
enum class LogLevel { Debug, Info, Warning, Critical };

// Exposed as c2.EventType
// Represents callbacks c2 can do into lua world
enum class EventType {
    CompletionRequested,
};

/**
 * This is for custom completion, a registered function returns this type
 * however in Lua array part (value) and object part (hideOthers) are in the same
 * table.
 */
struct CompletionList {
    std::vector<QString> values{};

    // exposed as hide_others
    bool hideOthers{};
};

}  // namespace chatterino::lua::api

#endif
