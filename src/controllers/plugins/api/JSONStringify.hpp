#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/**
 * Stringify a Lua value as JSON. Only tables and scalars (strings/numbers/booleans) are supported.
 *
 * Empty tables are stringified as objects. To get an empty array, use the following: `{ [0] = c2.json_null }` (will produce `[]`).
 * Tables with `nil` values like `{ foo = nil }` will be stringified as `{}` (they are identical to the empty table). To get `null` there,
 * use `c2.json_null`: `{ foo = c2.json_null }` (produces `{"foo":null}`).
 *
 * @lua@param input any The value to stringify
 * @lua@param opts? {pretty?: boolean, indent_char?: string, indent_size?: number} Additional options for stringifying. The default pretty indent char is a space and the size is 4.
 * @lua@return string
 * @exposed c2.json_stringify
 */
int jsonStringify(lua_State *L);

/* @lua-fragment
---Helper type to inidicate a `null` value when serializing.
---This is useful if `nil` would hide the value (such as in tables).
---See `c2.json_stringify` for more info.
---@type lightuserdata
c2.json_null = {}
*/

}  // namespace chatterino::lua::api

#endif
