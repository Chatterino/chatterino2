-- SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

local function make_emote(name)
    return c2.Emote.new_uncached({
        name = name,
        images = c2.ImageSet.new("https://example.com"),
        tooltip = "a tooltip",
    })
end

local tests = {
    emote_ctor = function()
        local images = c2.ImageSet.new("https://example.com")
        local emote = c2.Emote.new_uncached({
            name = "minimal",
            images = images,
            tooltip = "a",
        })
        assert(emote.name == "minimal")
        assert(emote.images == images)
        assert(emote.tooltip == "a")
        assert(emote.home_page == "")
        assert(not emote.zero_width)
        assert(emote.id == "")
        assert(emote.base_name == nil)

        emote = c2.Emote.new_uncached({
            name = "full",
            images = images,
            tooltip = "a",
            home_page = "https://example.com",
            zero_width = true,
            id = "1234",
            base_name = "basename",
        })
        assert(emote.name == "full")
        assert(emote.images == images)
        assert(emote.tooltip == "a")
        assert(emote.home_page == "https://example.com")
        assert(emote.zero_width)
        assert(emote.id == "1234")
        assert(emote.base_name == "basename")

        -- Missing items
        local ok, err = pcall(function()
            c2.Emote.new_uncached({ name = "foo" })
        end)
        assert(not ok)

        ok = pcall(function()
            c2.Emote.new_uncached({ name = "foo", images = images })
        end)
        assert(not ok)

        ok = pcall(function()
            c2.Emote.new_uncached({ name = "foo", tooltip = "foo" })
        end)
        assert(not ok)

        ok = pcall(function()
            c2.Emote.new_uncached({ images = images, tooltip = "foo" })
        end)
        assert(not ok)

        -- Invalid home_page
        ok, err = pcall(function()
            c2.Emote.new_uncached({
                name = "foo",
                images = images,
                tooltip = "foo",
                home_page = "file:///usr/bin/shutdown",
            })
        end)
        assert(not ok and err == "`home_page` must be an http(s) link")
    end,
    emote_equality = function()
        local emote1 = make_emote("foo")
        local emote2 = make_emote("bar")
        assert(emote1 == emote1)
        assert(emote2 == emote2)
        assert(emote1 ~= emote2)
        assert(emote1 == make_emote("foo"))
        assert(emote2 == make_emote("bar"))

        local is1 = c2.ImageSet.new("https://example.com")
        local is2 = c2.ImageSet.new("https://example.com/2")
        local emote3 = c2.Emote.new_uncached({
            name = "foo",
            images = is1,
            tooltip = "tool",
            home_page = "https://example.com",
            zero_width = true,
        })
        -- Only name, images, tooltip, and home_page are checked.
        assert(emote3 == c2.Emote.new_uncached({
            name = "foo",
            images = is1,
            tooltip = "tool",
            home_page = "https://example.com",
            zero_width = false,
            author = "author",
            base_name = "base",
            id = "1234",
        }))
        assert(emote3 ~= c2.Emote.new_uncached({
            name = "foo2",
            images = is1,
            tooltip = "tool",
            home_page = "https://example.com",
        }))
        assert(emote3 ~= c2.Emote.new_uncached({
            name = "foo",
            images = is2,
            tooltip = "tool",
            home_page = "https://example.com",
        }))
        assert(emote3 ~= c2.Emote.new_uncached({
            name = "foo",
            images = is1,
            tooltip = "tool2",
            home_page = "https://example.com",
        }))
        assert(emote3 ~= c2.Emote.new_uncached({
            name = "foo",
            images = is1,
            tooltip = "tool",
            home_page = "https://example.com/2",
        }))
    end,
    emote_element = function()
        local emote = make_emote("foo")
        local msg = c2.Message.new({
            elements = {
                { type = "emote", emote = emote, flags = c2.MessageElementFlag.Emote },
                { type = "emote", emote = emote, flags = c2.MessageElementFlag.Emote, text_element_color = "system" },
            },
        })
        assert(msg:elements()[1].emote == emote)
        assert(msg:elements()[1].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[1].text_element_color == "text")
        assert(msg:elements()[2].emote == emote)
        assert(msg:elements()[2].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[2].text_element_color == "system")
    end,
    layered_emote_element = function()
        local emote1 = make_emote("foo")
        local emote2 = make_emote("foo2")
        local msg = c2.Message.new({
            elements = {
                {
                    type = "layered-emote",
                    emotes = {
                        { emote = emote1, flags = c2.MessageElementFlag.Emote },
                        { emote = emote2, flags = c2.MessageElementFlag.EmoteImage },
                    },
                    flags = c2.MessageElementFlag.Emote,
                },
                {
                    type = "layered-emote",
                    emotes = {
                        { emote = emote1, flags = c2.MessageElementFlag.Emote },
                    },
                    flags = c2.MessageElementFlag.Emote,
                    text_element_color = "link",
                },
            },
        })
        assert(#msg:elements()[1].emotes == 2)
        assert(msg:elements()[1].emotes[1].emote == emote1)
        assert(msg:elements()[1].emotes[1].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[1].emotes[2].emote == emote2)
        assert(msg:elements()[1].emotes[2].flags == c2.MessageElementFlag.EmoteImage)
        assert(msg:elements()[1].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[1].text_element_color == "text")

        assert(#msg:elements()[2].emotes == 1)
        assert(msg:elements()[2].emotes[1].emote == emote1)
        assert(msg:elements()[2].emotes[1].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[2].flags == c2.MessageElementFlag.Emote)
        assert(msg:elements()[2].text_element_color == "link")

        local ok, err = pcall(c2.Message.new, {
            elements = {
                { type = "layered-emote", emotes = {}, flags = c2.MessageElementFlag.Emote },
            },
        })
        assert(not ok and err == "At least one emote must be added")
    end,
}

for name, fn in pairs(tests) do
    local ok, res = xpcall(fn, debug.traceback)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
