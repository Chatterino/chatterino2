#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/LuaUtilities.hpp"

#    include "common/Channel.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"

#    include <lauxlib.h>
#    include <lua.h>

#    include <climits>
#    include <cstdlib>

namespace chatterino::lua {

void stackDump(lua_State *L, const QString &tag)
{
    qCDebug(chatterinoLua) << "--------------------";
    auto count = lua_gettop(L);
    if (!tag.isEmpty())
    {
        qCDebug(chatterinoLua) << "Tag: " << tag;
    }
    qCDebug(chatterinoLua) << "Count elems: " << count;
    for (int i = 1; i <= count; i++)
    {
        auto typeint = lua_type(L, i);
        if (typeint == LUA_TSTRING)
        {
            QString str;
            lua::peek(L, &str, i);
            qCDebug(chatterinoLua)
                << "At" << i << "is a" << lua_typename(L, typeint) << "("
                << typeint << "): " << str;
        }
        else if (typeint == LUA_TTABLE)
        {
            qCDebug(chatterinoLua)
                << "At" << i << "is a" << lua_typename(L, typeint) << "("
                << typeint << ")"
                << "its length is " << lua_rawlen(L, i);
        }
        else
        {
            qCDebug(chatterinoLua)
                << "At" << i << "is a" << lua_typename(L, typeint) << "("
                << typeint << ")";
        }
    }
    qCDebug(chatterinoLua) << "--------------------";
}

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
    return lua::push(L, str.toStdString());
}

StackIdx push(lua_State *L, const std::string &str)
{
    lua_pushstring(L, str.c_str());
    return lua_gettop(L);
}

StackIdx push(lua_State *L, const CommandContext &ctx)
{
    auto outIdx = pushEmptyTable(L, 2);

    push(L, ctx.words);
    lua_setfield(L, outIdx, "words");
    push(L, ctx.channel->getName());
    lua_setfield(L, outIdx, "channel_name");

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

bool peek(lua_State *L, std::string *out, StackIdx idx)
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
    *out = std::string(str, len);
    return true;
}

QString toString(lua_State *L, StackIdx idx)
{
    size_t len{};
    const auto *ptr = luaL_tolstring(L, idx, &len);
    return QString::fromUtf8(ptr, int(len));
}
}  // namespace chatterino::lua
#endif
