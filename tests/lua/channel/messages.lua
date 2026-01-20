-- SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

local chan = c2.Channel.by_name("mm2pl")
assert(chan)

---@param ... c2.Message[]
local function add_all(...)
    for _, msg in ipairs({ ... }) do
        chan:add_message(msg)
    end
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
}

for name, fn in pairs(tests) do
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
