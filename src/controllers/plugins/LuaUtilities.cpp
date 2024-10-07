#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/LuaUtilities.hpp"

#    include "common/QLogging.hpp"

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

StackIdx push(lua_State *L, const QString &str)
{
    return lua::push(L, str.toStdString());
}

StackIdx push(lua_State *L, const std::string &str)
{
    lua_pushstring(L, str.c_str());
    return lua_gettop(L);
}

bool peek(lua_State *L, QString *out, StackIdx idx)
{
    StackGuard guard(L);
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

QString toString(lua_State *L, StackIdx idx)
{
    size_t len{};
    const auto *ptr = luaL_tolstring(L, idx, &len);
    return QString::fromUtf8(ptr, int(len));
}
}  // namespace chatterino::lua
#endif
