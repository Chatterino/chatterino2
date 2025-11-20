local json = require('chatterino.json')

local function compare_v(lhs, rhs)
    if type(lhs) == "table" then
        if type(rhs) ~= "table" then
            print("lhs:", type(lhs), "rhs:", type(rhs))
            return false
        end
        for key_l, val_l in pairs(lhs) do
            local val_r = rhs[key_l]
            if not compare_v(val_l, val_r) then
                print("[", key_l, "] failed")
                return false
            end
        end
        for key_r, _ in pairs(rhs) do
            if lhs[key_r] == nil then
                print(key_r, "was in rhs but not in lhs")
                return false
            end
        end
        return true
    end

    if lhs ~= rhs then
        print("lhs:", lhs, "rhs:", rhs)
        return false
    end
    return true
end

local function check_err(...)
    local ok = pcall(json.parse, ...)
    local arg0 = ...
    if arg0 == nil then
        arg0 = "(nil)"
    end
    assert(not ok, "parsing '" .. arg0 .. "' should've resulted in an error")
end

---@param input string
---@param expected any
---@vararg {allow_comments?: boolean, allow_trailing_commas?: boolean}|any
local function check_ok(input, expected, ...)
    local parsed = json.parse(input, ...)
    assert(compare_v(parsed, expected), "parsing '" .. input .. "' didn't yield the expected value")
end

-- empty
check_ok('{}', {})
check_ok('[]', {})
check_ok('{}', {}, { allow_comments = true })
check_ok('{}', {}, { allow_trailing_commas = true })
check_ok('{}', {}, { allow_comments = true, allow_trailing_commas = true })

-- spaces
check_ok('  {}  ', {})
check_ok('  {\n\t}  ', {})
check_ok('  {\r\n\t}  ', {})
check_ok('  {}\n\n  \n', {})


-- scalars
check_ok('""', "")
check_ok('"foo"', "foo")
check_ok('"foo\\nbar\\nbaz"', "foo\nbar\nbaz")
check_ok('true', true)
check_ok('false', false)
check_ok('0', 0)
check_ok('-0', 0)
check_ok('42', 42)
check_ok('-42', -42)
check_ok('1.23456', 1.23456)
check_ok('null', json.null)

-- integer limits
check_ok('9223372036854775807', 9223372036854775807) -- max int (this has to be equal)
-- +/- 1 precision in 64 bit float
local v_1_shl_64 = json.parse('18446744073709551615')
assert(v_1_shl_64 >= 18446744073709551614 or v_1_shl_64 <= 18446744073709551616)

check_ok('9223372036854775808', 9223372036854775808) -- double

-- objects
check_ok('{"foo": 1}', { foo = 1 })
check_ok('{"foo": [1, 2, {"bar": false}]}', { foo = { 1, 2, { bar = false } } })
check_ok('{"foo": 1, "bar": "hey", "baz": {"obj": true}, "a": null}',
    { foo = 1, bar = "hey", baz = { obj = true }, a = json.null })
check_ok('{"esc\\naped": "a string"}', { ["esc\naped"] = "a string" })
check_ok('{"": 1}', { [""] = 1 })
check_ok('{"": {"": 0}, "": 0}', { [""] = 0 })

-- arrays
check_ok('[]', {})
check_ok('[1]', { 1 })
check_ok('["string"]', { "string" })
check_ok('[null]', { json.null })
check_ok('[null, 1, null]', { json.null, 1, json.null })
check_ok('[{}]', { {} })
check_ok('[{}, {}]', { {}, {} })
check_ok('[{}, "foo", 1, false, true]', { {}, "foo", 1, false, true })
check_ok('[[[[[[{"foo": 1}]]]]]]', { { { { { { { foo = 1 } } } } } } })

-- From Qt
check_ok([[
[
    "json Test Pattern pass1",
    {"object with 1 member":["array with 1 element"]},
    {},
    [],
    -42,
    true,
    false,
    null,
    {
        "integer": 1234567890,
        "real": -9876.543210,
        "e": 0.123456789e-2,
        "E": 1.234567890E+3,
        "":  2E66,
        "zero": 0,
        "one": 1,
        "space": " ",
        "quote": "\"",
        "backslash": "\\",
        "controls": "\b\f\n\r\t",
        "slash": "/ & \/",
        "alpha": "abcdefghijklmnopqrstuvwxyz",
        "ALPHA": "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "digit": "0123456789",
        "0123456789": "digit",
        "special": "`1~!@#$%^&*()_+-={':[,]}|;.</>?",
        "hex": "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
        "true": true,
        "false": false,
        "null": null,
        "array":[  ],
        "object":{  },
        "address": "50 St. James Street",
        "url": "nix",
        "comment": "// /* <!-- --",
        "# -- --> */": " ",
        " s p a c e d " :[1,2 , 3

,

4 , 5        ,          6           ,7        ],"compact":[1,2,3,4,5,6,7],
        "jsontext": "{\"object with 1 member\":[\"array with 1 element\"]}",
        "quotes": "&#34; \u0022 %22 0x22 034 &#x22;",
        "\/\"\uCAFE\uBABE\uAB98\uFCDE\ubcda\uef4A\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?" : "A key can be any string"
    },
    0.5 ,98.6
,
99.44
,

1066,
1e1,
0.1e1,
1e-1,
1e00,
2e+00,
2e-00,
"rosebud",
{"foo": "bar"}
]

]], {
    "json Test Pattern pass1",
    { ["object with 1 member"] = { "array with 1 element" } },
    {},
    {},
    -42,
    true,
    false,
    json.null,
    {
        integer = 1234567890,
        real = -9876.543210,
        e = 0.123456789e-2,
        E = 1.234567890E+3,
        [""] = 2E66,
        zero = 0,
        one = 1,
        space = " ",
        quote = "\"",
        backslash = "\\",
        controls = "\b\f\n\r\t",
        slash = "/ & /",
        alpha = "abcdefghijklmnopqrstuvwxyz",
        ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        digit = "0123456789",
        ["0123456789"] = "digit",
        special = "`1~!@#$%^&*()_+-={\':[,]}|;.</>?",
        hex = "\u{0123}\u{4567}\u{89AB}\u{CDEF}\u{abcd}\u{ef4A}",
        ["true"] = true,
        ["false"] = false,
        null = json.null,
        array = {},
        object = {},
        address = "50 St. James Street",
        url = "nix",
        comment = "// /* <!-- --",
        ["# -- --> */"] = " ",
        [" s p a c e d "] = { 1, 2, 3, 4, 5, 6, 7 },
        compact = { 1, 2, 3, 4, 5, 6, 7 },
        jsontext = "{\"object with 1 member\":[\"array with 1 element\"]}",
        quotes = "&#34; \u{0022} %22 0x22 034 &#x22;",
        ["/\"\u{CAFE}\u{BABE}\u{AB98}\u{FCDE}\u{bcda}\u{ef4A}\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?"] =
        "A key can be any string"
    },
    0.5,
    98.6,
    99.44,
    1066,
    1e1,
    0.1e1,
    1e-1,
    1e00,
    2e+00,
    2e-00,
    "rosebud",
    { foo = "bar" }
})

-- limits
local big = string.rep('[', 256) .. string.rep(']', 256)
json.parse(big)

local too_big = string.rep('[', 257) .. string.rep(']', 257)
check_err(too_big)

big = string.rep('{"x":', 256) .. "1" .. string.rep('}', 256)
json.parse(big)

too_big = string.rep('{"x":', 257) .. "1" .. string.rep('}', 257)
check_err(too_big)

-- bad inputs
check_err('')
check_err('{} junk')
check_err('junk{}')
check_err('}')
check_err('{')
check_err('{"foo": 1')
check_err('{"foo": }')
check_err('{"foo": nul}')
check_err('"')
check_err('\\"foo"')
check_err()

-- comments
local bcpl_comment = [[
    {
        "my val": 42, // a comment
        "other": false
    }
]]
check_ok(bcpl_comment, { ["my val"] = 42, other = false }, { allow_comments = true })
check_ok(bcpl_comment, { ["my val"] = 42, other = false }, { allow_comments = true, allow_trailing_commas = false })
check_ok(bcpl_comment, { ["my val"] = 42, other = false }, { allow_comments = true, allow_trailing_commas = true })
check_err(bcpl_comment)
check_err(bcpl_comment, { allow_comments = false })
check_err(bcpl_comment, { allow_trailing_commas = false })
check_err(bcpl_comment, { allow_trailing_commas = true })
check_err(bcpl_comment, { allow_comments = false, allow_trailing_commas = true })

local c_comment = [[
    {
        "my val": 42, /* a comment
        */"other": false
    }
]]
check_ok(c_comment, { ["my val"] = 42, other = false }, { allow_comments = true })
check_ok(c_comment, { ["my val"] = 42, other = false }, { allow_comments = true, allow_trailing_commas = false })
check_ok(c_comment, { ["my val"] = 42, other = false }, { allow_comments = true, allow_trailing_commas = true })
check_err(c_comment)
check_err(c_comment, { allow_comments = false })
check_err(c_comment, { allow_trailing_commas = false })
check_err(c_comment, { allow_trailing_commas = true })
check_err(c_comment, { allow_comments = false, allow_trailing_commas = true })

local trailing_comma = [[
    {
        "my val": 42,
        "other": false,
    }
]]
check_ok(trailing_comma, { ["my val"] = 42, other = false }, { allow_trailing_commas = true })
check_ok(trailing_comma, { ["my val"] = 42, other = false }, { allow_trailing_commas = true, allow_comments = false })
check_ok(trailing_comma, { ["my val"] = 42, other = false }, { allow_trailing_commas = true, allow_comments = true })
check_err(trailing_comma)
check_err(trailing_comma, { allow_trailing_commas = false })
check_err(trailing_comma, { allow_comments = false })
check_err(trailing_comma, { allow_comments = true })
check_err(trailing_comma, { allow_trailing_commas = false, allow_comments = true })

local trailing_comma_comment = [[
    {
        "my val": 42, // a comment
        "other": false, /* another comment */
    }
]]
check_ok(trailing_comma_comment, { ["my val"] = 42, other = false },
    { allow_trailing_commas = true, allow_comments = true })
check_err(trailing_comma_comment, { allow_trailing_commas = true })
check_err(trailing_comma_comment, { allow_trailing_commas = true, allow_comments = false })
check_err(trailing_comma_comment)
check_err(trailing_comma_comment, { allow_trailing_commas = false })
check_err(trailing_comma_comment, { allow_comments = false })
check_err(trailing_comma_comment, { allow_comments = true })
check_err(trailing_comma_comment, { allow_trailing_commas = false, allow_comments = true })
