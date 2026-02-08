-- SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

local chan = c2.Channel.by_name("mm2pl")
local last_handle = nil ---@type c2.ConnectionHandle|nil
assert(chan)

---@param ... c2.Message[]
local function add_all(...)
    for _, msg in ipairs({ ... }) do
        chan:add_message(msg)
    end
end

---@param h c2.ConnectionHandle|nil
local function remember_handle(h)
    if last_handle ~= nil then
        last_handle:disconnect()
    end
    last_handle = h
end

local tests = {
    message_snapshot = function()
        local snap = chan:message_snapshot(10)
        assert(type(snap) == "userdata")
        assert(#snap == 0)

        chan:add_system_message("system message")
        snap = chan:message_snapshot(10)
        assert(#snap == 1)

        local msgs = {}
        for i = 1, 10 do
            local msg = c2.Message.new({ id = tostring(i), message_text = "something" })
            chan:add_message(msg)
            table.insert(msgs, msg)
        end

        snap = chan:message_snapshot(10)
        assert(#snap == 10)
        for i = 1, 10 do
            assert(msgs[i] == snap[i])
        end
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

        chan:replace_message(msg4, msg5, 4)
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

        chan:replace_message(msg5, msg3, 4) -- hint is one-based
        snap = chan:message_snapshot(2)
        assert(snap[1] == msg5 and snap[2] == msg3)
    end,
    replace_message_at = function()
        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2" })
        local msg3 = c2.Message.new({ id = "3" })
        local msg4 = c2.Message.new({ id = "4" })
        local msg5 = c2.Message.new({ id = "5" })

        chan:replace_message_at(1, msg1)
        chan:replace_message_at(2, msg1)
        chan:replace_message_at(3, msg1)

        add_all(msg1, msg2, msg3, msg4)

        chan:replace_message_at(1, msg5)
        local snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg4)

        chan:replace_message_at(4, msg5)
        snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg5)

        chan:replace_message_at(5, msg5)
        snap = chan:message_snapshot(4)
        assert(snap[1] == msg5 and snap[2] == msg2 and snap[3] == msg3 and snap[4] == msg5)
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

        local found = chan:find_message_by_id("4")
        assert(found ~= nil)
        chan:add_message(found)
        assert(chan:find_message_by_id("4") == found)
        assert(chan:find_message_by_id("4") == msg4_dup)
        assert(chan:find_message_by_id("4") ~= msg4)
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
    on_message_appended = function()
        chan:add_system_message("first")
        local last_msg = nil
        local last_flags = nil
        remember_handle(chan:on_message_appended(function(msg, flags)
            last_msg = msg
            last_flags = flags
        end))
        chan:add_system_message("second")
        assert(last_msg and last_msg.message_text == "second")
        assert(last_flags == nil)
        local msg = c2.Message.new({ id = "42" })
        chan:add_message(msg, c2.MessageContext.Original, c2.MessageFlag.Timeout)
        assert(last_msg and last_msg == msg)
        assert(last_flags and last_flags == c2.MessageFlag.Timeout)
    end,
    on_message_appended_recursion = function()
        local i = 0
        remember_handle(chan:on_message_appended(function(msg, flags)
            i = i + 1
            chan:add_system_message(tostring(i))
        end))
        chan:add_system_message("start")
        assert(chan:message_snapshot(1)[1].message_text == "62")
        assert(i == 63)
    end,
    on_message_replaced = function()
        local msg1 = c2.Message.new({ id = "1" })
        local msg2 = c2.Message.new({ id = "2" })
        local msg3 = c2.Message.new({ id = "3" })
        local msg4 = c2.Message.new({ id = "4" })
        add_all(msg1, msg2, msg3)
        local last_idx = nil
        local last_old = nil
        local last_replacement = nil
        remember_handle(chan:on_message_replaced(function(idx, old, replacement)
            -- make sure that the message at `idx` always points to the new message
            local snap = chan:message_snapshot(100)
            assert(snap[idx] == replacement)

            last_idx = idx
            last_old = old
            last_replacement = replacement
        end))
        chan:replace_message_at(3, msg4)
        assert(last_idx == 3)
        assert(last_old == msg3)
        assert(last_replacement == msg4)
        chan:replace_message_at(1, msg3)
        assert(last_idx == 1)
        assert(last_old == msg1)
        assert(last_replacement == msg3)
    end,
    on_message_replaced_recursion = function()
        local i = 0
        remember_handle(chan:on_message_replaced(function(idx, old, rep)
            i = i + 1
            chan:replace_message_at(idx, c2.Message.new({ id = tostring(i) }))
        end))
        chan:add_system_message("start")
        chan:replace_message_at(1, c2.Message.new({ id = "whatever" }))
        assert(chan:message_snapshot(1)[1].id == "62")
        assert(i == 63)
    end,
    on_messages_cleared = function()
        chan:add_system_message("something")
        local called = false
        remember_handle(chan:on_messages_cleared(function()
            called = true
        end))
        assert(not called)
        chan:clear_messages()
        assert(called)
        called = false
        chan:clear_messages()
        assert(called)
    end,
    on_messages_cleared_recursion = function()
        local calls = 0
        remember_handle(chan:on_messages_cleared(function()
            calls = calls + 1
            chan:clear_messages()
        end))
        chan:clear_messages()
        assert(calls == 63) -- we're still alive
    end,
}

for name, fn in pairs(tests) do
    remember_handle(nil) -- clear last handle
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
