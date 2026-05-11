// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <lua.h>
#    include <lualib.h>
#    include <magic_enum/magic_enum.hpp>
#    include <QList>
#    include <sol/state_view.hpp>

#    include <cassert>
#    include <string>
#    include <string_view>
#    include <type_traits>
struct lua_State;

namespace chatterino::lua {

/**
 * @brief Dumps the Lua stack into qCDebug(chatterinoLua)
 *
 * @param tag is a string to let you know which dump is which when browsing logs
 */
void stackDump(lua_State *L, const QString &tag);

// This is for calling stackDump out of gdb as it's not easy to create a QString there
const QString GDB_DUMMY = "GDB_DUMMY";

/**
 * @brief Converts a lua error code and potentially string on top of the stack into a human readable message
 */
QString humanErrorText(lua_State *L, int errCode);

/**
 * Represents an index into Lua's stack
 */
using StackIdx = int;

StackIdx push(lua_State *L, const QString &str);
StackIdx push(lua_State *L, const std::string &str);

// returns OK?
bool peek(lua_State *L, QString *out, StackIdx idx = -1);

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
        if (this->expected < 0)
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

/**
 * @brief Creates a table mapping enum names to unique values.
 *
 * Values in this table may change.
 *
 * @returns Sol reference to the table
 */
template <typename T, T... Additional>
    requires std::is_enum_v<T>
sol::table createEnumTable(sol::state_view &lua)
{
    constexpr auto values = magic_enum::enum_values<T>();
    auto out = lua.create_table(0, values.size() + sizeof...(Additional));
    for (const T v : values)
    {
        out.raw_set(magic_enum::enum_name<T>(v), v);
    }
    (out.raw_set(magic_enum::enum_name<Additional>(), Additional), ...);

    return out;
}

/// luaL_error but with [[noreturn]]
[[noreturn]]
void fail(lua_State *L, const char *fmt, auto &&...args)
{
    luaL_error(L, fmt, std::forward<decltype(args)>(args)...);
    // luaL_error is not annotated with [[noreturn]] for backwards
    // compatibility, but it will never return. If we ever get here, something
    // is seriously wrong, so abort.
    std::terminate();
}

}  // namespace chatterino::lua

#endif
