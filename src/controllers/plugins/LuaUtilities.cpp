#include "LuaUtilities.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/Channel.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "lauxlib.h"
#    include "lua.h"

#    include <climits>
#    include <cstdlib>

namespace chatterino::lua {

QString humanErrorText(lua_State *L, int errCode)
{
    QString errName;
    switch (errCode)
    {
        case LUA_OK:
            return "ok";
        case LUA_ERRRUN:
            errName = "(runtime error)";
            break;
        case LUA_ERRMEM:
            errName = "(memory error)";
            break;
        case LUA_ERRERR:
            errName = "(error while handling another error)";
            break;
        case LUA_ERRSYNTAX:
            errName = "(syntax error)";
            break;
        case LUA_YIELD:
            errName = "(illegal coroutine yield)";
            break;
        case LUA_ERRFILE:
            errName = "(file error)";
            break;
        default:
            errName = "(unknown error type)";
    }
    QString errText;
    if (peek(L, &errText))
    {
        errName += " " + errText;
    }
    return errName;
}

StackIdx pushEmptyArray(lua_State *L, int countArray)
{
    lua_createtable(L, countArray, 0);
    return lua_gettop(L);
}

StackIdx pushEmptyTable(lua_State *L, int countProperties)
{
    lua_createtable(L, 0, countProperties);
    return lua_gettop(L);
}

StackIdx push(lua_State *L, const QString &str)
{
    lua_pushstring(L, str.toStdString().c_str());
    return lua_gettop(L);
}

StackIdx push(lua_State *L, const CommandContext &ctx)
{
    auto outIdx = pushEmptyTable(L, 2);

    push(L, ctx.words);
    lua_setfield(L, outIdx, "words");
    push(L, ctx.channel->getName());
    lua_setfield(L, outIdx, "channelName");

    return outIdx;
}

StackIdx push(lua_State *L, const bool &b)
{
    lua_pushboolean(L, int(b));
    return lua_gettop(L);
}

bool peek(lua_State *L, double *out, StackIdx idx)
{
    int ok{0};
    auto v = lua_tonumberx(L, idx, &ok);
    if (ok != 0)
    {
        *out = v;
    }
    return ok != 0;
}

bool peek(lua_State *L, QString *out, StackIdx idx)
{
    size_t len{0};
    const char *str = lua_tolstring(L, idx, &len);
    if (str == nullptr)
    {
        return false;
    }
    if (len >= INT_MAX)
    {
        assert(false && "string longer than INT_MAX, shit's fucked, yo");
    }
    *out = QString::fromUtf8(str, int(len));
    return true;
}

bool peek(lua_State *L, QByteArray *out, StackIdx idx)
{
    size_t len{0};
    const char *str = lua_tolstring(L, idx, &len);
    if (str == nullptr)
    {
        return false;
    }
    if (len >= INT_MAX)
    {
        assert(false && "string longer than INT_MAX, shit's fucked, yo");
    }
    *out = QByteArray(str, int(len));
    return true;
}
}  // namespace chatterino::lua
#endif
