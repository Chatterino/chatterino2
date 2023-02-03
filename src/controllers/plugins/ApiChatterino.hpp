#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

struct lua_State;
namespace chatterino::lua::api {
// names in this namespace reflect what's visible inside Lua and follow the lua naming scheme

int c2_register_command(lua_State *L);  // NOLINT(readability-identifier-naming)
int c2_send_msg(lua_State *L);          // NOLINT(readability-identifier-naming)
int c2_system_msg(lua_State *L);        // NOLINT(readability-identifier-naming)

int g_load(lua_State *L);  // NOLINT(readability-identifier-naming)

}  // namespace chatterino::lua::api

#endif
