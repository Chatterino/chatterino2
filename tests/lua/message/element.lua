-- SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

local chan = c2.Channel.by_name("mm2pl")
assert(chan)

local tests = {
    append_element = function()
        local msg = c2.Message.new({
            elements = {
                {
                    type = "text",
                    text = "abcde",
                },
                { type = "twitch-moderation" },
            },
        })
        chan:add_message(msg)
        local new_msg = c2.Message.new(msg)
        local append_init_ok, append_init_err = pcall(function()
            new_msg:append_element({
                type = "text",
                text = "foo",
            })
        end)
        assert(append_init_ok)
        assert(new_msg:elements()[1])
        assert(new_msg:elements()[1].type == "text" and new_msg:elements()[1].words[1] == "foo")
        local append_element_ok, append_element_err = pcall(function()
            for index, element in ipairs(msg:elements()) do
                new_msg:append_element(element)
            end
        end)
        assert(append_element_ok)
        assert(new_msg:elements()[2])
        assert(new_msg:elements()[2].type == "text" and new_msg:elements()[2].words[1] == "abcde")
    end,
    exhaustive_flags = function()
        local msg = c2.Message.new({
            elements = {
                {
                    type = "text",
                    text = "a",
                    -- default
                },
                {
                    type = "text",
                    text = "b",
                    exhaustive_flags = false,
                },
                {
                    type = "text",
                    text = "c",
                    exhaustive_flags = true,
                },
            },
        })
        assert(msg:elements()[1].type == "text" and msg:elements()[1].exhaustive_flags == false)
        assert(msg:elements()[2].type == "text" and msg:elements()[2].exhaustive_flags == false)
        assert(msg:elements()[3].type == "text" and msg:elements()[3].exhaustive_flags == true)
        local cloned = msg:clone()
        assert(cloned:elements()[1].type == "text" and cloned:elements()[1].exhaustive_flags == false)
        assert(cloned:elements()[2].type == "text" and cloned:elements()[2].exhaustive_flags == false)
        assert(cloned:elements()[3].type == "text" and cloned:elements()[3].exhaustive_flags == true)
    end,
}

for name, fn in pairs(tests) do
    chan:clear_messages() -- start off without any messages

    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
