#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "lua.h"
#    include "lualib.h"

#    include <magic_enum.hpp>
#    include <QList>

#    include <string>
#    include <string_view>
#    include <type_traits>
#    include <vector>
struct lua_State;
class QJsonObject;
namespace chatterino {
struct CommandContext;
}  // namespace chatterino

namespace chatterino::lua {

/**
 * @brief Converts a lua error code and potentially string on top of the stack into a human readable message
 */
QString humanErrorText(lua_State *L, int errCode);

using StackIdx = int;

StackIdx pushEmptyArray(lua_State *L, int countArray);
StackIdx pushEmptyTable(lua_State *L, int countProperties);

StackIdx push(lua_State *L, const CommandContext &ctx);
StackIdx push(lua_State *L, const QString &str);
StackIdx push(lua_State *L, const std::string &str);
StackIdx push(lua_State *L, const bool &b);

// returns OK?
bool peek(lua_State *L, double *out, StackIdx idx = -1);
bool peek(lua_State *L, QString *out, StackIdx idx = -1);
bool peek(lua_State *L, QByteArray *out, StackIdx idx = -1);
bool peek(lua_State *L, std::string *out, StackIdx idx = -1);

// forces conversion of value at idx to a string
QString toString(lua_State *L, StackIdx idx = -1);

/// TEMPLATES

template <typename T,
          typename std::enable_if<std::is_enum_v<T>, bool>::type = true>
bool peek(lua_State *L, T *out, StackIdx idx = -1)
{
    std::string tmp;
    if (!lua::peek(L, &tmp, idx))
    {
        return false;
    }
    std::optional<T> opt = magic_enum::enum_cast<T>(tmp);
    if (opt.has_value())
    {
        *out = opt.value();
        return true;
    }

    return false;
}

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

template <typename T, std::enable_if<std::is_enum_v<T>>>
StackIdx push(lua_State *L, T inp)
{
    std::string_view name = magic_enum::enum_name<T>(inp);
    return lua::push(L, std::string(name));
}

template <typename T>
bool pop(lua_State *L, T *out, StackIdx idx = -1)
{
    auto ok = peek(L, out, idx);
    if (ok)
    {
        lua_pop(L, 1);
    }
    return ok;
}

template <typename T>
StackIdx pushEnumTable(lua_State *L)
{
    // std::array<T, _>
    auto values = magic_enum::enum_values<T>();
    StackIdx out = lua::pushEmptyTable(L, values.size());
    for (const T v : values)
    {
        std::string_view name = magic_enum::enum_name<T>(v);
        std::string str(name);

        lua::push(L, str);
        lua_setfield(L, out, str.c_str());
    }
    return out;
}

}  // namespace chatterino::lua
#endif
