---@generic T
---@param a T
---@param b T
local function assert_eq(a, b)
    if a ~= b then
        error(tostring(a) .. " ~= " .. tostring(b))
    end
end

local specs_ms = {
    {
        iso_ms = "2026-07-10T20:14:41.477Z",
        iso_sec = "2026-07-10T20:14:41Z",
        unix_ms = 1783714481477,
        unix_sec = 1783714481,
    },
    {
        iso_ms = "1930-11-18T00:28:29.123Z",
        iso_sec = "1930-11-18T00:28:29Z",
        unix_ms = -1234567890877,
        unix_sec = -1234567890,
    },
}
local specs_sec = {
    {
        iso_ms = "2026-07-10T20:14:41.000Z",
        iso_sec = "2026-07-10T20:14:41Z",
        unix_ms = 1783714481000,
        unix_sec = 1783714481,
    },
    {
        iso_ms = "1930-11-18T00:28:29.000Z",
        iso_sec = "1930-11-18T00:28:29Z",
        unix_ms = -1234567891000,
        unix_sec = -1234567891,
    },
}

local tests = {
    iso_string_invalid = function()
        local dt = c2.DateTime.from_iso_string("invalid")
        assert_eq(dt:to_unix_milliseconds(), 0)
        assert_eq(dt:to_iso_string(), "")
    end,
    iso_string_ms = function()
        for _, spec in pairs(specs_ms) do
            local dt = c2.DateTime.from_iso_string(spec.iso_ms)

            assert_eq(dt, c2.DateTime.from_iso_string(spec.iso_ms))
            assert_eq(dt:to_iso_string(), tostring(dt))
            assert_eq(dt:to_iso_string(), spec.iso_ms)
            assert_eq(dt:to_iso_string_without_ms(), spec.iso_sec)
            assert_eq(dt:to_unix_milliseconds(), spec.unix_ms)
            assert_eq(dt:to_unix_seconds(), spec.unix_sec)
            assert(dt:is_utc())
        end
    end,
    iso_string_sec = function()
        for _, spec in pairs(specs_sec) do
            local dt = c2.DateTime.from_iso_string(spec.iso_sec)

            assert_eq(dt, c2.DateTime.from_iso_string(spec.iso_ms))
            assert_eq(dt, c2.DateTime.from_iso_string(spec.iso_sec))
            assert_eq(dt:to_iso_string(), tostring(dt))
            assert_eq(dt:to_iso_string(), spec.iso_ms)
            assert_eq(dt:to_iso_string_without_ms(), spec.iso_sec)
            assert_eq(dt:to_unix_milliseconds(), spec.unix_ms)
            assert_eq(dt:to_unix_seconds(), spec.unix_sec)
            assert(dt:is_utc())
        end
    end,

    current_time = function()
        local dt = c2.DateTime.current_local()
        -- We don't know what it should be, but we know that it must be valid.
        assert(dt:to_iso_string() ~= "")
        assert(dt:is_local())
        dt = c2.DateTime.current_utc()
        assert(dt:to_iso_string() ~= "")
        assert(dt:is_utc())
    end,

    unix_timestamp_ms = function()
        for _, spec in pairs(specs_ms) do
            local dt = c2.DateTime.from_unix_milliseconds(spec.unix_ms)
            assert_eq(dt, c2.DateTime.from_unix_milliseconds(spec.unix_ms))
            assert_eq(dt:to_unix_milliseconds(), spec.unix_ms)
            assert_eq(dt:to_unix_seconds(), spec.unix_sec)
            assert(dt:is_local())

            assert_eq(dt, c2.DateTime.from_iso_string(spec.iso_ms))
            assert_eq(dt:to_utc():to_iso_string(), spec.iso_ms)
            assert_eq(dt:to_utc():to_iso_string_without_ms(), spec.iso_sec)
        end
    end,
    unix_timestamp_sec = function()
        for _, spec in pairs(specs_sec) do
            -- without milliseconds
            local dt = c2.DateTime.from_unix_seconds(spec.unix_sec)
            assert_eq(dt, c2.DateTime.from_unix_seconds(spec.unix_sec))
            assert_eq(dt, c2.DateTime.from_unix_milliseconds(spec.unix_ms))
            assert_eq(dt:to_unix_milliseconds(), spec.unix_ms)
            assert_eq(dt:to_unix_seconds(), spec.unix_sec)
            assert(dt:is_local())

            assert_eq(dt, c2.DateTime.from_iso_string(spec.iso_ms))
            assert_eq(dt:to_utc():to_iso_string(), spec.iso_ms)
            assert_eq(dt:to_utc():to_iso_string_without_ms(), spec.iso_sec)
        end
    end,

    local_utc_conversion = function()
        local dt_utc = c2.DateTime.current_utc()
        local dt_local = dt_utc:to_local()
        assert(dt_utc:is_utc())
        assert(dt_local:is_local())
        assert_eq(dt_local, dt_utc)

        dt_local = c2.DateTime.current_local()
        dt_utc = dt_local:to_utc()
        assert(dt_utc:is_utc())
        assert(dt_local:is_local())
        assert_eq(dt_local, dt_utc)
    end,
}

for name, fn in pairs(tests) do
    local ok, res = xpcall(fn, debug.traceback)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
