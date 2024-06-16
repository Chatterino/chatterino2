#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/LuaUtilities.hpp"

#    include "common/Channel.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/LuaAPI.hpp"

extern "C" {
#    include <lauxlib.h>
#    include <lua.h>
}

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
        case ERROR_BAD_PEEK:
            errName = "(unable to convert value to c++)";
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
    StackGuard guard(L, 1);
    auto outIdx = pushEmptyTable(L, 2);

    push(L, ctx.words);
    lua_setfield(L, outIdx, "words");

    push(L, ctx.channel);
    lua_setfield(L, outIdx, "channel");

    return outIdx;
}

StackIdx push(lua_State *L, const bool &b)
{
    lua_pushboolean(L, int(b));
    return lua_gettop(L);
}

StackIdx push(lua_State *L, const int &b)
{
    lua_pushinteger(L, b);
    return lua_gettop(L);
}

StackIdx push(lua_State *L, const api::CompletionEvent &ev)
{
    auto idx = pushEmptyTable(L, 4);
#    define PUSH(field)         \
        lua::push(L, ev.field); \
        lua_setfield(L, idx, #field)
    PUSH(query);
    PUSH(full_text_content);
    PUSH(cursor_position);
    PUSH(is_first_word);
#    undef PUSH
    return idx;
}

bool peek(lua_State *L, int *out, StackIdx idx)
{
    StackGuard guard(L);
    if (lua_isnumber(L, idx) == 0)
    {
        return false;
    }

    *out = lua_tointeger(L, idx);
    return true;
}

bool peek(lua_State *L, bool *out, StackIdx idx)
{
    StackGuard guard(L);
    if (!lua_isboolean(L, idx))
    {
        return false;
    }

    *out = bool(lua_toboolean(L, idx));
    return true;
}

bool peek(lua_State *L, double *out, StackIdx idx)
{
    StackGuard guard(L);
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

bool peek(lua_State *L, QByteArray *out, StackIdx idx)
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
    *out = QByteArray(str, int(len));
    return true;
}

bool peek(lua_State *L, std::string *out, StackIdx idx)
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
    *out = std::string(str, len);
    return true;
}

bool peek(lua_State *L, api::CompletionList *out, StackIdx idx)
{
    StackGuard guard(L);
    int typ = lua_getfield(L, idx, "values");
    if (typ != LUA_TTABLE)
    {
        lua_pop(L, 1);
        return false;
    }
    if (!lua::pop(L, &out->values, -1))
    {
        return false;
    }
    lua_getfield(L, idx, "hide_others");
    return lua::pop(L, &out->hideOthers);
}

QString toString(lua_State *L, StackIdx idx)
{
    size_t len{};
    const auto *ptr = luaL_tolstring(L, idx, &len);
    return QString::fromUtf8(ptr, int(len));
}
}  // namespace chatterino::lua
#endif
