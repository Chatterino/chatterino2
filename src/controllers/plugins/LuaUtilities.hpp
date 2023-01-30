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

}  // namespace chatterino::lua
