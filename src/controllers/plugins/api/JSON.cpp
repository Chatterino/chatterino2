#include "controllers/plugins/api/JSON.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/JSONParse.hpp"
#    include "controllers/plugins/api/JSONStringify.hpp"

#    include <sol/sol.hpp>

namespace {

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)
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

}  // namespace

namespace chatterino::lua::api {

sol::object loadJson(sol::state_view lua)
{
    // Set the metatable for all lightuserdata
    auto *L = lua.lua_state();
    lua_checkstack(L, 4);
    lua_pushlightuserdata(L, nullptr);
    lua_createtable(L, 0, 1);
    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, &stringifyLightuserdata);
    lua_rawset(L, -3);        // tbl.__tostring = fn
    lua_setmetatable(L, -2);  // setmeta(lightuserd) = tbl
    lua_pop(L, 1);            // pop lightuserdata

    return lua.create_table_with(    //
        "parse", jsonParse,          //
        "stringify", jsonStringify,  //
        // pushed as lightuserdata
        "null", static_cast<void *>(nullptr)  //
    );
}

}  // namespace chatterino::lua::api

#endif
