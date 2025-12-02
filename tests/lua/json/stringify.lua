local json = require('chatterino.json')

local function check_err(...)
    local ok = pcall(json.stringify, ...)
    assert(not ok, "expected an error")
end

---@param input any
---@param expected string|string[]
---@vararg { pretty: boolean, indent_char: string, indent_size: number }|any
local function check_ok(input, expected, ...)
    local got = json.stringify(input, ...)
    if type(expected) == "string" then
        assert(got == expected, "Failed:\n\texpected: " .. expected .. "\n\tgot: " .. got .. "\n")
    else
        local ok = false
        for _, it in ipairs(expected) do
            if it == got then
                ok = true
                break
            end
        end
        assert(ok, "Output did not match any of the expected values")
    end
end

assert(type(json.null) == "userdata")

-- empty
check_ok({}, "{}")
check_ok({ [0] = json.null }, '[]')

-- scalars
check_ok(1, "1")
check_ok("foo", '"foo"')
check_ok("foo\n", '"foo\\n"')
check_ok(true, "true")
check_ok(false, "false")
check_ok(nil, "null")
check_ok(json.null, "null")
check_ok(1.23, "1.23")

-- arrays
check_ok({ 1, 2 }, "[1,2]")
check_ok({ 1, 2, nil }, "[1,2]")
check_ok({ 1, 2, json.null }, "[1,2,null]")
check_ok({ 1, nil, 2 }, "[1,null,2]")
check_ok({ 1, [10] = 2 }, "[1,null,null,null,null,null,null,null,null,2]")
check_ok({ [10 / 2] = 1 }, '[null,null,null,null,1]')
check_ok({ { { { { 1 } } } } }, "[[[[[1]]]]]")
check_ok({ { { { { 1 } } }, { foo = 1 } } }, '[[[[[1]]],{"foo":1}]]')

-- objects
check_ok({ ok = { baz = "str", [""] = { 1, 2 } } }, {
    '{"ok":{"baz":"str","":[1,2]}}',
    '{"ok":{"":[1,2],"baz":"str"}}',
})
check_ok({ ok = { 1, 2 } }, '{"ok":[1,2]}')
check_ok({ ['\n\tlol'] = { 1, 2 } }, '{"\\n\\tlol":[1,2]}')
check_ok({ foo = nil }, '{}')
check_ok({ foo = json.null }, '{"foo":null}')

local meta_haver = {}
local my_meta = {
    foo = 42,
    __index = function()
        return 1
    end
}
setmetatable(meta_haver, my_meta)
assert(meta_haver.xd == 1)
check_ok(meta_haver, '{}')
meta_haver.xd = 2 -- uses __newindex
check_ok(meta_haver, '{"xd":2}')

-- limits
local big_obj = {}
; (function()
    local it = big_obj
    for _ = 1, 255 do
        it.x = {}
        it = it.x
    end
    it.x = "hey"
end)()
check_ok(big_obj, string.rep('{"x":', 256) .. '"hey"' .. string.rep('}', 256))
check_err({ x = big_obj })

local big_arr = {}
; (function()
    local it = big_arr
    for _ = 1, 255 do
        it[1] = {}
        it = it[1]
    end
    it[1] = 1
end)()
check_ok(big_arr, string.rep('[', 256) .. '1' .. string.rep(']', 256))
check_err({ big_arr })

-- errors
check_err(c2.Message.new({}))
check_err(coroutine.create(function() end))
check_err({ 1, 2, bar = "lol" })
check_err({ [-1] = 3 })
check_err({ [0] = 3 })
check_err({ [0] = "42" })
check_err({ [-1] = json.null })
check_err({ [1.1] = 3 })
check_err()
local recurse = {}
recurse.recurse = recurse
check_err(recurse) -- hits the depth limit

check_err(math.huge)
check_err(-math.huge)
check_err(0/0)
check_err(-(0/0))

-- ✨ pretty ✨

check_ok(1, "1", { pretty = true })
check_ok(false, "false", { pretty = true })
check_ok(nil, "null", { pretty = true })
check_ok("xd", '"xd"', { pretty = true })
check_ok({ 1 }, '[\n    1\n]', { pretty = true })
check_ok({ foo = "bar" }, '{\n    "foo": "bar"\n}', { pretty = true })
check_ok({ foo = "bar", baz = 1 }, { [[{
    "foo": "bar",
    "baz": 1
}]], [[{
    "baz": 1,
    "foo": "bar"
}]] }, { pretty = true })
check_ok({ foo = "bar" }, '{\n "foo": "bar"\n}', { pretty = true, indent_size = 1 })
check_ok({ foo = "bar" }, '{\n\t"foo": "bar"\n}', { pretty = true, indent_size = 1, indent_char = '\t' })
check_ok({ foo = "bar" }, '{"foo":"bar"}', { indent_size = 1, indent_char = '\tlol' })
