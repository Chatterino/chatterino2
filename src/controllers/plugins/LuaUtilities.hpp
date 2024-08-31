#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/QLogging.hpp"

extern "C" {
#    include <lua.h>
#    include <lualib.h>
}
#    include <magic_enum/magic_enum.hpp>
#    include <QList>

#    include <cassert>
#    include <optional>
#    include <string>
#    include <string_view>
#    include <type_traits>
#    include <variant>
#    include <vector>
struct lua_State;
class QJsonObject;
namespace chatterino {
struct CommandContext;
}  // namespace chatterino

namespace chatterino::lua {

namespace api {
    struct CompletionList;
    struct CompletionEvent;
}  // namespace api

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
StackIdx push(lua_State *L, const int &b);
StackIdx push(lua_State *L, const api::CompletionEvent &ev);

// returns OK?
bool peek(lua_State *L, int *out, StackIdx idx = -1);
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

// This object ensures that the stack is of expected size when it is destroyed
class StackGuard
{
    int expected;
    lua_State *L;

public:
    /**
     * Use this constructor if you expect the stack size to be the same on the
     * destruction of the object as its creation
     */
    StackGuard(lua_State *L)
        : expected(lua_gettop(L))
        , L(L)
    {
    }

    /**
     * Use this if you expect the stack size changing, diff is the expected difference
     * Ex: diff=3 means three elements added to the stack
     */
    StackGuard(lua_State *L, int diff)
        : expected(lua_gettop(L) + diff)
        , L(L)
    {
    }

    ~StackGuard()
    {
        if (expected < 0)
        {
            return;
        }
        int after = lua_gettop(this->L);
        if (this->expected != after)
        {
            stackDump(this->L, "StackGuard check tripped");
            // clang-format off
            // clang format likes to insert a new line which means that some builds won't show this message fully
            assert(false && "internal error: lua stack was not in an expected state");
            // clang-format on
        }
    }

    // This object isn't meant to be passed around
    StackGuard operator=(StackGuard &) = delete;
    StackGuard &operator=(StackGuard &&) = delete;
    StackGuard(StackGuard &) = delete;
    StackGuard(StackGuard &&) = delete;

    // This function tells the StackGuard that the stack isn't in an expected state but it was handled
    void handled()
    {
        this->expected = -1;
    }
};

/// TEMPLATES

template <typename T>
StackIdx push(lua_State *L, std::optional<T> val)
{
    if (val.has_value())
    {
        return lua::push(L, *val);
    }
    lua_pushnil(L);
    return lua_gettop(L);
}

template <typename T>
bool peek(lua_State *L, std::optional<T> *out, StackIdx idx = -1)
{
    if (lua_isnil(L, idx))
    {
        *out = std::nullopt;
        return true;
    }

    *out = T();
    return peek(L, out->operator->(), idx);
}

template <typename T>
bool peek(lua_State *L, std::vector<T> *vec, StackIdx idx = -1)
{
    StackGuard guard(L);

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
            //lua_seti(L, LUA_REGISTRYINDEX, 1);  // lazy
            qCDebug(chatterinoLua)
                << "Failed to convert lua object into c++: at array index " << i
                << ":";
            stackDump(L, "bad conversion into string");
            return false;
        }
        lua_pop(L, 1);
        vec->push_back(obj.value());
    }
    return true;
}

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
template <typename T, typename std::enable_if_t<std::is_enum_v<T>, bool> = true>
StackIdx push(lua_State *L, T inp)
{
    std::string_view name = magic_enum::enum_name<T>(inp);
    return lua::push(L, std::string(name));
}

/**
 * @brief Converts a Lua object into c++ and removes it from the stack.
 * If peek fails, the object is still removed from the stack.
 *
 * Relies on bool peek(lua_State*, T*, StackIdx) existing.
 */
template <typename T>
bool pop(lua_State *L, T *out, StackIdx idx = -1)
{
    StackGuard guard(L, -1);
    auto ok = peek(L, out, idx);
    if (idx < 0)
    {
        idx = lua_gettop(L) + idx + 1;
    }
    lua_remove(L, idx);
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
    StackIdx stackIdx_;
    lua_State *L;

public:
    CallbackFunction(lua_State *L, StackIdx stackIdx)
        : stackIdx_(stackIdx)
        , L(L)
    {
    }

    // this type owns the stackidx, it must not be trivially copiable
    CallbackFunction operator=(CallbackFunction &) = delete;
    CallbackFunction(CallbackFunction &) = delete;

    // Permit only move
    CallbackFunction &operator=(CallbackFunction &&) = default;
    CallbackFunction(CallbackFunction &&) = default;

    ~CallbackFunction()
    {
        lua_remove(L, this->stackIdx_);
    }

    std::variant<int, ReturnType> operator()(Args... arguments)
    {
        lua_pushvalue(this->L, this->stackIdx_);
        (  // apparently this calls lua::push() for every Arg
            [this, &arguments] {
                lua::push(this->L, arguments);
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
