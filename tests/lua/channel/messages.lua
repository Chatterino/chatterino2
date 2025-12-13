local chan = c2.Channel.by_name("mm2pl")
assert(chan)

---@param getter fun(n: number): c2.Message[]
local function msg_snapshot_base(getter)
    local snap = getter(10)
    assert(#snap == 0)

    chan:add_system_message("system message")
    snap = getter(10)
    assert(#snap == 1)

    local msgs = {}
    for i = 1, 10 do
        local msg = c2.Message.new({ id = tostring(i), message_text = "something" })
        chan:add_message(msg)
        table.insert(msgs, msg)
    end

    snap = getter(10)
    assert(#snap == 10)
    for i = 1, 10 do
        assert(msgs[i] == snap[i])
    end
end

---@param ... c2.Message[]
local function add_all(...)
    for _, msg in ipairs({ ... }) do
        chan:add_message(msg)
    end
end

local tests = {
    message_snapshot = function()
        assert(type(chan:message_snapshot(10)) == "userdata")

        msg_snapshot_base(function(n)
            return chan:message_snapshot(n)
        end)
    end,
    message_snapshot_as_table = function()
        assert(type(chan:message_snapshot_as_table(10)) == "table")

        msg_snapshot_base(function(n)
            return chan:message_snapshot_as_table(n)
        end)
    end,
    last_message = function()
        assert(chan:last_message() == nil)
        local msg = c2.Message.new({ id = "42" })
        chan:add_message(msg)
        assert(chan:last_message() == msg)

        msg = c2.Message.new({ id = "43" })
        chan:add_message(msg)
        assert(chan:last_message() == msg)
    end,
    disable_all_messages = function()
        chan:disable_all_messages()

        local Flag = c2.MessageFlag

        local msg_no_flags = c2.Message.new({})
        local msg_sys = c2.Message.new({ flags = Flag.System })
        local msg_timeout = c2.Message.new({ flags = Flag.Timeout })
        local msg_whisper = c2.Message.new({ flags = Flag.Whisper })
        local msg_sys_timeout = c2.Message.new({ flags = Flag.System | Flag.Timeout })
        local msg_disabled = c2.Message.new({ flags = Flag.Disabled })
        local msg_sub = c2.Message.new({ flags = Flag.Subscription })
        add_all(msg_no_flags, msg_sys, msg_timeout, msg_whisper, msg_sys_timeout, msg_disabled, msg_sub)

        assert(msg_no_flags.flags == 0)
        assert(msg_sys.flags == Flag.System)
        assert(msg_timeout.flags == Flag.Timeout)
        assert(msg_whisper.flags == Flag.Whisper)
        assert(msg_sys_timeout.flags == (Flag.System | Flag.Timeout))
        assert(msg_disabled.flags == Flag.Disabled)
        assert(msg_sub.flags == Flag.Subscription)

        chan:disable_all_messages()

        assert(msg_no_flags.flags == Flag.Disabled)
        assert(msg_sys.flags == Flag.System)
        assert(msg_timeout.flags == Flag.Timeout)
        assert(msg_whisper.flags == Flag.Whisper)
        assert(msg_sys_timeout.flags == (Flag.System | Flag.Timeout))
        assert(msg_disabled.flags == Flag.Disabled)
        assert(msg_sub.flags == Flag.Subscription | Flag.Disabled)
    end,
    replace_message = function()
        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2" })
        local msg3 = c2.Message.new({ id = "3" })
        local msg4 = c2.Message.new({ id = "4" })
        local msg5 = c2.Message.new({ id = "5" })

        chan:replace_message(msg3, msg4) -- noop

        add_all(msg1, msg2, msg3, msg4)

        chan:replace_message(msg3, msg5)

        local snap = chan:message_snapshot(2)
        assert(snap[1] == msg5 and snap[2] == msg4)

        chan:replace_message(msg4, msg5, 3)
        snap = chan:message_snapshot(2)
        assert(snap[1] == msg5 and snap[2] == msg5)

        -- test how we handle duplicate messages
        -- currently we replace the first message
        chan:replace_message(msg5, msg3)
        snap = chan:message_snapshot(2)
        assert(snap[1] == msg3 and snap[2] == msg5)

        -- reset and try with a hint
        chan:replace_message(msg3, msg5)
        snap = chan:message_snapshot(2)
        assert(snap[1] == msg5 and snap[2] == msg5)

        chan:replace_message(msg5, msg3, 3) -- hint is zero-based
        snap = chan:message_snapshot(2)
        assert(snap[1] == msg5 and snap[2] == msg3)
    end,
    replace_message_at = function()
        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2" })
        local msg3 = c2.Message.new({ id = "3" })
        local msg4 = c2.Message.new({ id = "4" })
        local msg5 = c2.Message.new({ id = "5" })

        chan:replace_message_at(0, msg1)
        chan:replace_message_at(1, msg1)
        chan:replace_message_at(2, msg1)

        add_all(msg1, msg2, msg3, msg4)

        chan:replace_message_at(0, msg5)
        local snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg4)

        chan:replace_message_at(3, msg5)
        snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg5)

        chan:replace_message_at(4, msg5)
        snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg5)
    end,
    disable_message_by_id = function()
        local Flag = c2.MessageFlag

        chan:disable_message_by_id("42")

        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2", flags = Flag.System })
        local msg3 = c2.Message.new({ id = "3", flags = Flag.Subscription | Flag.EventSub })

        add_all(msg1, msg2, msg3)

        chan:disable_message_by_id("3")
        assert(msg1.flags == 0)
        assert(msg2.flags == Flag.System)
        assert(msg3.flags == Flag.Subscription | Flag.EventSub | Flag.Disabled)

        chan:disable_message_by_id("2")
        assert(msg1.flags == 0)
        assert(msg2.flags == Flag.System | Flag.Disabled)
        assert(msg3.flags == Flag.Subscription | Flag.EventSub | Flag.Disabled)

        chan:disable_message_by_id("1")
        assert(msg1.flags == Flag.Disabled)
        assert(msg2.flags == Flag.System | Flag.Disabled)
        assert(msg3.flags == Flag.Subscription | Flag.EventSub | Flag.Disabled)

        chan:disable_message_by_id("4")
        assert(msg1.flags == Flag.Disabled)
        assert(msg2.flags == Flag.System | Flag.Disabled)
        assert(msg3.flags == Flag.Subscription | Flag.EventSub | Flag.Disabled)
    end,
    clear_messages = function()
        chan:clear_messages()
        assert(not chan:has_messages())
        assert(chan:count_messages() == 0)

        chan:add_system_message("msg")
        chan:add_system_message("msg")
        chan:add_system_message("msg")
        chan:add_message(c2.Message.new({}))

        assert(chan:has_messages())
        assert(chan:count_messages() == 4)
        chan:clear_messages()

        assert(not chan:has_messages())
        assert(chan:count_messages() == 0)
    end,
    find_message_by_id = function()
        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2" })
        local msg3 = c2.Message.new({ id = "3" })
        local msg4 = c2.Message.new({ id = "4" })

        assert(chan:find_message_by_id("1") == nil)

        add_all(msg1, msg2, msg3, msg4)
        assert(chan:find_message_by_id("1") == msg1)
        assert(chan:find_message_by_id("2") == msg2)
        assert(chan:find_message_by_id("3") == msg3)
        assert(chan:find_message_by_id("4") == msg4)
        assert(chan:find_message_by_id("5") == nil)
        assert(chan:find_message_by_id("") == nil)

        local msg4_dup = c2.Message.new({ id = "4" })
        chan:add_message(msg4_dup)
        assert(chan:find_message_by_id("4") == msg4_dup)
    end,
    has_messages = function()
        assert(not chan:has_messages())
        chan:add_system_message("1")
        assert(chan:has_messages())
        chan:add_system_message("2")
        assert(chan:has_messages())
    end,
    count_messages = function()
        assert(chan:count_messages() == 0)
        chan:add_system_message("1")
        assert(chan:count_messages() == 1)
        chan:add_system_message("2")
        assert(chan:count_messages() == 2)
    end,
}

for name, fn in pairs(tests) do
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
