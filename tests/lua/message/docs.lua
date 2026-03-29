-- These tests mirror the code example from the docs - make sure they work
local tests = {
    frozen_messages = function()
        local my_msg = c2.Message.new({ id = "foobar" })
        assert(not my_msg.frozen)
        local chan = c2.Channel.by_name("mm2pl")
        assert(chan)
        chan:add_message(my_msg)
        assert(my_msg.frozen)
    end,
    element_access = function()
        local my_msg = c2.Message.new({
            id = "foo",
            elements = {
                { type = "text", text = "foo" },
                { type = "text", text = "bar" },
                { type = "text", text = "baz" },
            }
        })
        local elements = my_msg:elements()
        assert(#elements == 3)
        assert(elements[1].words[1] == "foo")
        assert(elements[2].words[1] == "bar")

        elements:erase(2) -- erase "bar"
        assert(#elements == 2)
        assert(elements[1].words[1] == "foo")
        assert(elements[2].words[1] == "baz")
    end,
    append_element = function()
        local my_msg = c2.Message.new({ id = "foo" })
        local els = my_msg:elements()
        assert(#els == 0)
        my_msg:append_element({
            type = "text",
            text = "My text element",
        })
        assert(#els == 1)
        assert(els[1].type == "text")
    end
}

for name, fn in pairs(tests) do
    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
