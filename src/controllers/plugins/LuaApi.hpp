#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

struct lua_State;
namespace chatterino::lua::api {
// names in this namespace reflect what's visible inside Lua and follow the lua naming scheme

// NOLINTBEGIN(readability-identifier-naming)
int c2_register_command(lua_State *L);
int c2_send_msg(lua_State *L);
int c2_system_msg(lua_State *L);

int g_load(lua_State *L);
int g_dofile(lua_State *L);
// NOLINTEND(readability-identifier-naming)

}  // namespace chatterino::lua::api

#endif
