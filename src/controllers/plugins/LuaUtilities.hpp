#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/CompletionModel.hpp"
#    include "common/QLogging.hpp"

#    include <boost/variant/variant.hpp>
#    include <lua.h>
#    include <lualib.h>
#    include <magic_enum.hpp>
#    include <QList>

#    include <memory>
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

constexpr int ERROR_BAD_PEEK = LUA_OK - 1;
/**
 * @brief Dumps the Lua stack into qCDebug(chatterinoLua)
 *
 * @param tag is a string to let you know which dump is which when browsing logs
 */
void stackDump(lua_State *L, const QString &tag);

/**
 * @brief Converts a lua error code and potentially string on top of the stack into a human readable message
 */
QString humanErrorText(lua_State *L, int errCode);

/**
 * Represents an index into Lua's stack
 */
using StackIdx = int;

// This would be in its own file but c++ seems to get confused about the peek() implementation so it's here.
// - Mm2PL
namespace api {
    /**
     * This is for custom completion, a registered functions returns this type
     * however in Lua array part (value) and object part (done) are in the same
     * table.
     */
    struct CompletionList {
        std::vector<std::pair<QString, CompletionModel::TaggedString::Type>>
            value;
        bool done;
    };
}  // namespace api

/**
 * @brief Creates a table with countArray array properties on the Lua stack
 * @return stack index of the newly created table
 */
StackIdx pushEmptyArray(lua_State *L, int countArray);

/**
 * @brief Creates a table with countProperties named properties on the Lua stack
 * @return stack index of the newly created table
 */
StackIdx pushEmptyTable(lua_State *L, int countProperties);

StackIdx push(lua_State *L, const CommandContext &ctx);
StackIdx push(lua_State *L, const QString &str);
StackIdx push(lua_State *L, const std::string &str);
StackIdx push(lua_State *L, const bool &b);

// returns OK?
bool peek(lua_State *L, bool *out, StackIdx idx = -1);
bool peek(lua_State *L, double *out, StackIdx idx = -1);
bool peek(lua_State *L, QString *out, StackIdx idx = -1);
bool peek(lua_State *L, QByteArray *out, StackIdx idx = -1);
bool peek(lua_State *L, std::string *out, StackIdx idx = -1);
bool peek(lua_State *L, api::CompletionList *out, StackIdx idx = -1);
/**
 * @brief Converts Lua object at stack index idx to a string.
 */
QString toString(lua_State *L, StackIdx idx = -1);

/// TEMPLATES

/**
 * @brief Converts object at stack index idx to enum given by template parameter T
 */
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

template <typename T, typename U>
bool peek(lua_State *L, std::optional<std::pair<T, U>> *opt, StackIdx idx = -1)
{
    if (!lua_istable(L, idx))
    {
        return false;
    }
    auto len = lua_rawlen(L, idx);
    if (len != 2)  // a pair has two elements, duh
    {
        return false;
    }

    T left;
    U right;
    lua_geti(L, idx, 1);
    if (!lua::peek(L, &left))
    {
        lua_seti(L, LUA_REGISTRYINDEX, 1);  // lazy
        qCDebug(chatterinoLua)
            << "Failed to convert lua object into c++: at pair left:";
        lua_getglobal(L, "print");
        lua_geti(L, LUA_REGISTRYINDEX, 1);
        lua_call(L, 1, 0);
        return false;
    }
    lua_pop(L, 1);

    lua_geti(L, idx, 2);
    if (!lua::peek(L, &right))
    {
        lua_seti(L, LUA_REGISTRYINDEX, 1);  // lazy
        qCDebug(chatterinoLua)
            << "Failed to convert lua object into c++: at pair right:";
        lua_getglobal(L, "print");
        lua_geti(L, LUA_REGISTRYINDEX, 1);
        lua_call(L, 1, 0);
        return false;
    }
    lua_pop(L, 1);
    opt->emplace(left, right);
    return true;
}

template <typename T>
bool peek(lua_State *L, std::vector<T> *vec, StackIdx idx = -1)
{
    if (!lua_istable(L, idx))
    {
        lua::stackDump(L, "!table");
        qCDebug(chatterinoLua)
            << "value is not a table, type is" << lua_type(L, idx);
        return false;
    }
    auto len = lua_rawlen(L, idx);
    if (len == 0)
    {
        qCDebug(chatterinoLua) << "value has 0 length";
        return true;
    }
    if (len > 1'000'000)
    {
        qCDebug(chatterinoLua) << "value is too long";
        return false;
    }
    // count like lua
    for (int i = 1; i <= len; i++)
    {
        lua_geti(L, idx, i);
        std::optional<T> obj;
        if (!lua::peek(L, &obj))
        {
            lua_seti(L, LUA_REGISTRYINDEX, 1);  // lazy
            qCDebug(chatterinoLua)
                << "Failed to convert lua object into c++: at array index " << i
                << ":";
            lua_getglobal(L, "print");
            lua_geti(L, LUA_REGISTRYINDEX, 1);
            lua_call(L, 1, 0);
            return false;
        }
        lua_pop(L, 1);
        vec->push_back(obj.value());
    }
    return true;
}

/**
 * @brief Converts a vector<T> to Lua and pushes it onto the stack.
 *
 * Needs StackIdx push(lua_State*, T); to work.
 *
 * @return Stack index of newly created table.
 */
template <typename T>
StackIdx push(lua_State *L, std::vector<T> vec)
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

/**
 * @brief Converts a QList<T> to Lua and pushes it onto the stack.
 *
 * Needs StackIdx push(lua_State*, T); to work.
 *
 * @return Stack index of newly created table.
 */
template <typename T>
StackIdx push(lua_State *L, QList<T> vec)
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

/**
 * @brief Converts an enum given by T to Lua (into a string) and pushes it onto the stack.
 *
 * @return Stack index of newly created string.
 */
template <typename T, std::enable_if<std::is_enum_v<T>>>
StackIdx push(lua_State *L, T inp)
{
    std::string_view name = magic_enum::enum_name<T>(inp);
    return lua::push(L, std::string(name));
}

/**
 * @brief Converts a Lua object into c++ and removes it from the stack.
 *
 * Relies on bool peek(lua_State*, T*, StackIdx) existing.
 */
template <typename T>
bool pop(lua_State *L, T *out, StackIdx idx = -1)
{
    auto ok = peek(L, out, idx);
    if (ok)
    {
        if (idx < 0)
        {
            idx = lua_gettop(L) + idx + 1;
        }
        lua_remove(L, idx);
    }
    return ok;
}

/**
 * @brief Creates a table mapping enum names to unique values.
 *
 * Values in this table may change.
 *
 * @returns stack index of newly created table
 */
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

// Represents a Lua function on the stack
template <typename ReturnType, typename... Args>
class CallbackFunction
{
    StackIdx stackidx;

public:
    CallbackFunction(StackIdx stackid)
        : stackidx(stackid)
    {
    }
    std::variant<int, ReturnType> operator()(lua_State *L, Args... arguments)
    {
        (  // apparently this calls lua::push() for every Arg
            [&L, &arguments] {
                lua::push(L, arguments);
            }(),
            ...);

        int res = lua_pcall(L, sizeof...(Args), 1, 0);
        if (res != LUA_OK)
        {
            qCDebug(chatterinoLua) << "error is: " << res;
            return {res};
        }

        ReturnType val;
        if (!lua::pop(L, &val))
        {
            return {ERROR_BAD_PEEK};
        }
        return {val};
    }
};

}  // namespace chatterino::lua

#endif
