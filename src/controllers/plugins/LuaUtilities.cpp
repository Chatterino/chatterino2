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

PeekResult peek(lua_State *L, int *out, StackIdx idx)
{
    StackGuard guard(L);
    if (lua_isnumber(L, idx) == 0)
    {
        return {
            false,
            {QString("Expected an integer got %1").arg(luaL_typename(L, idx))}};
    }

    *out = lua_tointeger(L, idx);
    return {};
}

PeekResult peek(lua_State *L, bool *out, StackIdx idx)
{
    StackGuard guard(L);
    if (!lua_isboolean(L, idx))
    {
        return {
            false,
            {QString("Expected a boolean got %1").arg(luaL_typename(L, idx))}};
    }

    *out = bool(lua_toboolean(L, idx));
    return {};
}

PeekResult peek(lua_State *L, double *out, StackIdx idx)
{
    StackGuard guard(L);
    int ok{0};
    auto v = lua_tonumberx(L, idx, &ok);
    if (ok != 0)
    {
        *out = v;
        return {};
    }

    return PeekResult::ofTypeError(L, idx, "integer or float");
}

PeekResult peek(lua_State *L, QString *out, StackIdx idx)
{
    StackGuard guard(L);
    size_t len{0};
    const char *str = lua_tolstring(L, idx, &len);
    if (str == nullptr)
    {
        return PeekResult::ofTypeError(L, idx, "string");
    }
    if (len >= INT_MAX)
    {
        return {false,
                {QString("Strings bigger than 2.1 gigabytes are not allowed")}};
    }
    *out = QString::fromUtf8(str, int(len));
    return {};
}

PeekResult peek(lua_State *L, QByteArray *out, StackIdx idx)
{
    StackGuard guard(L);
    size_t len{0};
    const char *str = lua_tolstring(L, idx, &len);
    if (str == nullptr)
    {
        return PeekResult::ofTypeError(L, idx, "string");
    }
    if (len >= INT_MAX)
    {
        return {false,
                {QString("Strings bigger than 2.1 gigabytes are not allowed")}};
    }
    *out = QByteArray(str, int(len));
    return {};
}

PeekResult peek(lua_State *L, std::string *out, StackIdx idx)
{
    StackGuard guard(L);
    size_t len{0};
    const char *str = lua_tolstring(L, idx, &len);
    if (str == nullptr)
    {
        return PeekResult::ofTypeError(L, idx, "string");
    }
    if (len >= INT_MAX)
    {
        return {false,
                {QString("Strings bigger than 2.1 gigabytes are not allowed")}};
    }
    *out = std::string(str, len);
    return {};
}

PeekResult peek(lua_State *L, api::CompletionList *out, StackIdx idx)
{
    StackGuard guard(L);
    int typ = lua_getfield(L, idx, "values");
    if (typ != LUA_TTABLE)
    {
        lua_pop(L, 1);
        auto res = PeekResult::ofTypeError(L, idx, "string");
        res.errorReason.emplace_back("While processing CompletionList.values");
        return res;
    }
    auto pres = lua::pop(L, &out->values, -1);
    if (!pres)
    {
        pres.errorReason.emplace_back("While processing CompletionList.values");
        return pres;
    }
    lua_getfield(L, idx, "hide_others");
    pres = lua::pop(L, &out->hideOthers);
    if (!pres)
    {
        pres.errorReason.emplace_back(
            "While processing CompletionList.hide_others");
    }
    return pres;
}

QString toString(lua_State *L, StackIdx idx)
{
    size_t len{};
    const auto *ptr = luaL_tolstring(L, idx, &len);
    return QString::fromUtf8(ptr, int(len));
}

void PeekResult::throwAsLuaError(lua_State *L)
{
    // This uses lua buffers to ensure deallocation of the error string
    luaL_Buffer buf;
    luaL_buffinit(L, &buf);
    bool first = true;
    for (const auto &s : this->errorReason)
    {
        if (!first)
        {
            luaL_addchar(&buf, '\n');
        }
        first = false;
        luaL_addstring(&buf, s.toStdString().c_str());
    }
    // Remove our copy
    this->errorReason.clear();

    luaL_pushresult(&buf);
    lua_error(L);  // This call never returns
    assert(false && "unreachable");
}

PeekResult PeekResult::ofTypeError(lua_State *L, StackIdx idx,
                                   const QString &expect)
{
    return PeekResult{
        .ok = false,
        .errorReason = {
            QString("Expected %1, got %2").arg(expect, luaL_typename(L, idx))}};
}
}  // namespace chatterino::lua
#endif
