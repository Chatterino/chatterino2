---@meta chatterino.json

local json = {}

--- Parse a string as JSON
---
---@param input string The text to parse
---@param opts? {allow_comments?: boolean, allow_trailing_commas?: boolean} Additional options for parsing. `allow_comments` will allow C++ style comments (`[1] // text`). `allow_trailing_commas` will allow commas before an object/array ends (`[1, 2,]`).
---@return any
function json.parse(input, opts) end

--- Stringify a Lua value as JSON. Only tables and scalars (strings/numbers/booleans) are supported.
--- Empty tables are stringified as objects. To get an empty array, use the following: `{ [0] = json.null }` (will produce `[]`).
--- Tables with `nil` values like `{ foo = nil }` will be stringified as `{}` (they are identical to the empty table). To get `null` there,
--- use `json.null`: `{ foo = json.null }` (produces `{"foo":null}`).
---
---@param input any The value to stringify
---@param opts? {pretty?: boolean, indent_char?: string, indent_size?: number} Additional options for stringifying. The default pretty indent char is a space and the size is 4.
---@return string
function json.stringify(input, opts) end

---Helper type to indicate a `null` value when serializing.
---This is useful if `nil` would hide the value (such as in tables).
---See `json.stringify` for more info.
---@type lightuserdata
json.null = {}

return json
