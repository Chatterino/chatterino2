-- SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

local chan = c2.Channel.by_name("mm2pl")
assert(chan)

local tests = {
    clone = function()
        local msg = c2.Message.new({
            flags = c2.MessageFlag.DoNotTriggerNotification,
            id = "123",
            parse_time = 12345678,
            search_text = "search",
            message_text = "message text",
            login_name = "login",
            display_name = "display",
            localized_name = "local",
            user_id = "user",
            channel_name = "channel",
            username_color = "#ff0000",
            server_received_time = 123345678,
            highlight_color = "#00ff00",
            elements = {
                { type = "text", text = "abcde" },
                { type = "twitch-moderation" },
            },
        })
        assert(not msg.frozen)
        chan:add_message(msg)
        assert(msg.frozen)
        local clone = msg:clone()
        assert(msg.frozen)
        assert(not clone.frozen)
        assert(msg ~= clone)
        assert(msg.flags == clone.flags)
        assert(msg.id == clone.id)
        assert(msg.parse_time == clone.parse_time)
        assert(msg.search_text == clone.search_text)
        assert(msg.message_text == clone.message_text)
        assert(msg.login_name == clone.login_name)
        assert(msg.display_name == clone.display_name)
        assert(msg.localized_name == clone.localized_name)
        assert(msg.user_id == clone.user_id)
        assert(msg.channel_name == clone.channel_name)
        assert(msg.username_color == clone.username_color)
        assert(msg.server_received_time == clone.server_received_time)
        assert(msg.highlight_color == clone.highlight_color)
        assert(#msg:elements() == #clone:elements())
    end,
}

for name, fn in pairs(tests) do
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
