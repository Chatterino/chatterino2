#pragma once

#include "lua.h"

#include <qlist.h>

#include <vector>
struct lua_State;
namespace chatterino {
class CommandContext;
}  // namespace chatterino

namespace chatterino::lua {

int pushEmptyArray(lua_State *L, int countArray);
int pushEmptyTable(lua_State *L, int countProperties);

int push(lua_State *L, const CommandContext &ctx);
int push(lua_State *L, const QString &str);
int push(lua_State *L, const bool &b);

// returns OK?
bool peek(lua_State *L, double *out, int idx = -1);
bool peek(lua_State *L, QString *out, int idx = -1);

/// TEMPLATES

template <typename T>
int push(lua_State *L, std::vector<T> vec)
{
    auto out = pushEmptyArray(L, vec.size());
    int i = 1;
    for (const auto &el : vec)
    {
        push(L, el);
        lua_seti(L, out, i);
        i += 1;
    }
    return out;
}

template <typename T>
int push(lua_State *L, QList<T> vec)
{
    auto out = pushEmptyArray(L, vec.size());
    int i = 1;
    for (const auto &el : vec)
    {
        push(L, el);
        lua_seti(L, out, i);
        i += 1;
    }
    return out;
}

template <typename T>
bool pop(lua_State *L, T *out, int idx = -1)
{
    auto ok = peek(L, out, idx);
    if (ok)
    {
        lua_pop(L, 1);
    }
    return ok;
}

}  // namespace chatterino::lua
