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
                { type = "text", text = "abc def" },
                { type = "twitch-moderation" },
            },
        })
        assert(not msg.frozen)
        chan:add_message(msg)
        assert(msg.frozen)
        local clone = msg:clone()

        for _, element in pairs(clone:elements()) do
            if element.type == "text" then
                local newWords = {}
                for k, word in pairs(element.words) do
                    newWords[k] = word:upper()
                end
                element.words = newWords
            end
        end

        assert(#msg:elements() == #clone:elements())

        assert(msg:elements()[1].words[1] == "abc")
        assert(msg:elements()[1].words[2] == "def")
        assert(clone:elements()[1].words[1] == "ABC")
        assert(clone:elements()[1].words[2] == "DEF")
    end,
}

for name, fn in pairs(tests) do
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
