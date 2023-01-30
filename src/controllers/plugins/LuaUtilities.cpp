#include "LuaUtilities.hpp"

#include "common/Channel.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "lua.h"

namespace chatterino::lua {
int pushEmptyArray(lua_State *L, int countArray)
{
    lua_createtable(L, countArray, 0);
    return lua_gettop(L);
}

int pushEmptyTable(lua_State *L, int countProperties)
{
    lua_createtable(L, 0, countProperties);
    return lua_gettop(L);
}

int push(lua_State *L, const QString &str)
{
    lua_pushstring(L, str.toStdString().c_str());
    return lua_gettop(L);
}

int push(lua_State *L, const CommandContext &ctx)
{
    auto outIdx = pushEmptyTable(L, 2);

    push(L, ctx.words);
    lua_setfield(L, outIdx, "words");
    push(L, ctx.channel->getName());
    lua_setfield(L, outIdx, "channelName");

    return outIdx;
}

}  // namespace chatterino::lua
