#include "controllers/plugins/api/JSONStringify.hpp"

#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <sol/sol.hpp>

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg) -- luaL_error is a vararg function

namespace {

constexpr size_t ENCODE_MAX_TABLE_LENGTH = 1 << 20;  // about 1 mil
constexpr uint16_t ENCODE_MAX_DEPTH = 256;

/// Get the size of an "array" (table at index -1)
///
/// Lua doesn't have a notion of arrays - everything is a table. We consider a
/// table an array if it only has positive non-zero integer keys. One notable
/// exception is an empty array. An empty table is considered an empty object.
/// If the user wants to specify an empty array, they specify a table with
/// `json_null` at item `0` (i.e. `{ [0] = c2.json_null }`). `json_null` is a
/// `nullptr` lightuserdata.
///
/// Arrays might have holes in them (e.g. `{1, [10]=2}` or `{1, nil, 2}`), so we
/// keep track of the maximum index we saw.
///
/// @returns The size of the array if  it is one - otherwise it's an object.
std::optional<size_t> inferArraySize(lua_State *L)
{
    lua_checkstack(L, 3);  // key + value + potential error

    size_t max = 0;
    bool hasNull = false;
    lua_pushnil(L);  // first key
    while (lua_next(L, -2) != 0)
    {
        // [-3] table
        // [-2] key
        // [-1] value
        if (lua_isinteger(L, -2) == 0)
        {
            lua_pop(L, 2);
            return std::nullopt;  // not an array
        }

        auto key = lua_tointeger(L, -2);
        if (key <= 0)
        {
            // special empty table indicator if element at 0 is our nullptr lightuserdata
            if (key == 0 && lua_islightuserdata(L, -1) &&
                lua_touserdata(L, -1) == nullptr)
            {
                hasNull = true;
            }
            else
            {
                luaL_error(L, "Table keys can't be negative integers");
                std::terminate();
            }
        }
        else
        {
            max = std::max(max, static_cast<size_t>(key));
        }

        lua_pop(L, 1);  // value
    }

    if (max > ENCODE_MAX_TABLE_LENGTH)
    {
        luaL_error(L, "Table is too big");
        std::terminate();
    }

    if (max == 0 && !hasNull)
    {
        return std::nullopt;  // empty tables are objects
    }

    return max;
}

void stringifyValue(lua_State *L, auto &writer, uint16_t depth);

void stringifyArray(lua_State *L, auto &writer, uint16_t depth, size_t length)
{
    lua_checkstack(L, 2);  // value + potential error

    writer.StartArray();

    for (size_t i = 0; i < length; i++)
    {
        lua_rawgeti(L, -1, static_cast<lua_Integer>(i) + 1);
        stringifyValue(L, writer, depth);
        lua_pop(L, 1);  // pop value
    }

    writer.EndArray();
}

void stringifyObject(lua_State *L, auto &writer, uint16_t depth)
{
    lua_checkstack(L, 3);  // key + value + potential error

    writer.StartObject();

    size_t items = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        items++;
        if (items > ENCODE_MAX_TABLE_LENGTH)
        {
            luaL_error(L, "Too many items in table");
            std::terminate();
        }
        // Stack:
        // [-3] table
        // [-2] key
        // [-1] value
        auto keyType = lua_type(L, -2);
        if (keyType != LUA_TSTRING)
        {
            luaL_error(L, "Object key was not a string, got %s",
                       lua_typename(L, keyType));
            std::terminate();
        }
        size_t len = 0;
        const char *str = lua_tolstring(L, -2, &len);
        writer.Key(str, len, /*copy=*/true);

        stringifyValue(L, writer, depth);
        lua_pop(L, 1);  // pop value
    }

    writer.EndObject();
}

void stringifyValue(lua_State *L, auto &writer, uint16_t depth)
{
    auto typeId = lua_type(L, -1);
    switch (typeId)
    {
        case LUA_TNIL:
            writer.Null();
            break;
        case LUA_TBOOLEAN:
            writer.Bool(lua_toboolean(L, -1) != 0);
            break;
        case LUA_TNUMBER: {
            if (lua_isinteger(L, -1))
            {
                writer.Int64(lua_tointeger(L, -1));
            }
            else
            {
                writer.Double(lua_tonumber(L, -1));
            }
        }
        break;
        case LUA_TSTRING: {
            size_t len = 0;
            const char *str = lua_tolstring(L, -1, &len);
            writer.String(str, len, /*copy=*/true);
        }
        break;
        case LUA_TTABLE: {
            if (depth >= ENCODE_MAX_DEPTH)
            {
                luaL_error(L, "Too deep");
                std::terminate();
            }

            auto arraySize = inferArraySize(L);
            if (arraySize)
            {
                stringifyArray(L, writer, depth + 1, *arraySize);
            }
            else
            {
                stringifyObject(L, writer, depth + 1);
            }
        }
        break;
        case LUA_TLIGHTUSERDATA: {
            auto *ptr = lua_touserdata(L, -1);
            if (ptr == nullptr)
            {
                writer.Null();
            }
            else
            {
                luaL_error(L, "Non-null lightuserdata not supported");
                std::terminate();
            }
        }
        break;
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
        default:
            luaL_error(L, "Unsupported type: %s", lua_typename(L, typeId));
            std::terminate();
    }
}

}  // namespace

namespace chatterino::lua::api {

int jsonStringify(lua_State *L)
{
    auto nArgs = lua_gettop(L);
    luaL_argcheck(L, nArgs >= 1, 1, "expected at at least one argument");

    bool pretty = false;
    char indentChar = ' ';
    unsigned int indentSize = 4;
    if (nArgs >= 2)
    {
        auto tbl = sol::stack::check_get<sol::table>(L);
        if (tbl)
        {
            pretty = tbl->get_or("pretty", pretty);
            indentChar = tbl->get_or<char>("indent_char", indentChar);
            indentSize = tbl->get_or("indent_size", indentSize);
        }

        // discard everything but the input
        lua_pop(L, nArgs - 1);
    }

    rapidjson::StringBuffer sb;
    if (pretty)
    {
        rapidjson::PrettyWriter pw(
            sb, static_cast<rapidjson::CrtAllocator *>(nullptr), 32);
        pw.SetIndent(indentChar, indentSize);
        stringifyValue(L, pw, 0);
    }
    else
    {
        rapidjson::Writer w(sb);
        stringifyValue(L, w, 0);
    }

    lua_pushlstring(L, sb.GetString(), sb.GetLength());
    return 1;
}

}  // namespace chatterino::lua::api

// NOLINTEND(cppcoreguidelines-pro-type-vararg)
