// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/api/JSON.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/JSONParse.hpp"
#    include "controllers/plugins/api/JSONStringify.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"

#    include <sol/sol.hpp>

namespace {

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)

/// `__tostring` implementation for lightuserdata
///
/// If the value is `nullptr`, returns "null", otherwise the pointer value in
/// hex (like in '%p').
int stringifyLightuserdata(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    void *ptr = lua_touserdata(L, 1);
    if (ptr == nullptr)
    {
        lua_pushstring(L, "null");
    }
    else
    {
        std::array<char, 2 + 16 + 1> buf{};
        // FIXME: use std::format once we require GCC 13/libstdc++ 13
        int written = std::snprintf(buf.data(), buf.size(), "%p", ptr);
        if (written < 0 || std::ranges::find(buf, '\0') == buf.end())
        {
            luaL_error(L, "Failed to format pointer");
        }
        lua_pushstring(L, buf.data());
    }
    return 1;
}
// NOLINTEND(cppcoreguidelines-pro-type-vararg)

/// Sets the global metatable for lightuserdata
///
/// `nullptr` is used as a sentinel for "null" values in both parsing and
/// stringifying. To allow users to get a proper string representation of
/// this value, we add a `__tostring` method on lightuserdata. As individual
/// lightuserdata values don't have metatables, this is done on the global
/// one.
void setLightuserdataMetatable(lua_State *L)
{
    chatterino::lua::StackGuard g(L);

    lua_checkstack(L, 4);

    lua_pushlightuserdata(L, nullptr);
    lua_createtable(L, 0, 1);
    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, &stringifyLightuserdata);

    // [-4] lightuserdata (nullptr)
    // [-3] table {}
    // [-2] "__tostring"
    // [-1] &stringifyLightuserdata
    lua_rawset(L, -3);  // tbl.__tostring = fn

    // [-2] lightuserdata (nullptr)
    // [-1] table { __tostring = fn }
    lua_setmetatable(L, -2);  // setmeta(lightuserdata) = tbl

    // [-1] lightuserdata (nullptr)
    lua_pop(L, 1);
}

}  // namespace

namespace chatterino::lua::api {

sol::object loadJson(sol::state_view lua)
{
    setLightuserdataMetatable(lua.lua_state());

    return lua.create_table_with(    //
        "parse", jsonParse,          //
        "stringify", jsonStringify,  //
        // pushed as lightuserdata
        "null", static_cast<void *>(nullptr)  //
    );
}

}  // namespace chatterino::lua::api

#endif
