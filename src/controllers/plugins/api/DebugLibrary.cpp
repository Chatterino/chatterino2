#include "controllers/plugins/api/DebugLibrary.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/sol.hpp>

namespace chatterino::lua::api {

/// Signature in Lua: ([thread,] [message [, level]])
int debugTraceback(lua_State *L)
{
    int argOffset = 1;
    lua_State *targetThread = L;

    // If the first argument is a thread, take that as our target
    if (lua_isthread(L, argOffset))
    {
        targetThread = lua_tothread(L, argOffset);
        argOffset += 1;
    }

    // The message is an optional string. If it's not a string/number/nil, we
    // return the message as-is and don't create a traceback.
    const char *msg = lua_tostring(L, argOffset);
    if (!msg && !lua_isnoneornil(L, argOffset))
    {
        lua_pushvalue(L, argOffset);
        return 1;
    }
    argOffset += 1;

    int defaultLevel = 0;
    if (L == targetThread)
    {
        defaultLevel = 1;
    }

    int level = static_cast<int>(luaL_optinteger(L, argOffset, defaultLevel));
    // push a traceback with `msg` at the start
    luaL_traceback(L, targetThread, msg, level);
    return 1;
}

}  // namespace chatterino::lua::api

#endif
