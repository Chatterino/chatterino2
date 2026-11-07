-- SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

---@type c2test.OwnedChannel[]
local _owned_channels = {}

---@param tbl table
---@return c2test.OwnedChannel
local function from_split_table(tbl)
    local c = c2test.OwnedChannel.from_split_table(tbl)
    table.insert(_owned_channels, c)
    return c
end

local function assert_deep_eq(lhs, rhs)
    if type(lhs) == "table" then
        if type(rhs) ~= "table" then
            error("lhs: " .. type(lhs) .. " ~= rhs: " .. type(rhs))
        end
        for key_l, val_l in pairs(lhs) do
            assert_deep_eq(val_l, rhs[key_l])
        end
        for key_r, _ in pairs(rhs) do
            if lhs[key_r] == nil then
                error(tostring(key_r) .. " was in `rhs` but not in `lhs`")
            end
        end
        return
    end

    if lhs ~= rhs then
        error("lhs: " .. tostring(lhs) .. " ~= rhs: " .. tostring(rhs))
    end
end

local tests = {
    basic = function()
        local before_owned = from_split_table({
            data = {
                type = "plugin",
                name = "testing-before",
                pluginID = "test",
                providerID = "test-provider",
                arguments = {
                    name = "dont-use-me",
                },
            },
        })
        assert(before_owned:channel():get_type() == c2.ChannelType.Plugin)

        local get_name_called = false
        local create_called = nil
        local destroy_called = nil
        c2.register_channel_provider({
            id = "test-provider",
            display_name = "display name",
            description = "just testing",
            arguments = {
                {
                    id = "name",
                    display_name = "Channel Name",
                    kind = "text",
                },
            },
            callbacks = {
                get_name = function(...)
                    get_name_called = true
                    error("should not be called")
                end,
                create = function(channel, args)
                    assert(create_called == nil)
                    create_called = { channel = channel, args = args }
                    local cc = { chan = channel, name = channel:get_name() }

                    function cc:on_send_message(msg)
                        self.chan:add_system_message(msg)
                    end
                    function cc:on_destroyed()
                        assert(destroy_called == nil)
                        destroy_called = cc.name
                    end

                    return cc
                end,
            },
        })

        assert(create_called ~= nil)
        assert(create_called.channel == before_owned:channel())
        assert_deep_eq(create_called.args, { arguments = { name = "dont-use-me" } })
        create_called = nil
        assert(before_owned:channel():get_name() == "testing-before")

        assert(before_owned:channel():count_messages() == 0)
        before_owned:channel():send_message("testing")
        assert(before_owned:channel():count_messages() == 1)
        assert(before_owned:channel():last_message().message_text == "testing")

        local after_owned = from_split_table({
            data = {
                type = "plugin",
                name = "testing-after",
                pluginID = "test",
                providerID = "test-provider",
                arguments = {
                    name = "not-used",
                },
            },
        })
        assert(after_owned:channel():get_name() == "testing-after")
        assert(create_called ~= nil)
        assert(create_called.channel ~= before_owned:channel())
        assert(create_called.channel == after_owned:channel())
        assert_deep_eq(create_called.args, { arguments = { name = "not-used" } })
        create_called = nil

        assert_deep_eq(before_owned:to_split_table().data, {
            type = "plugin",
            name = "testing-before",
            pluginID = "test",
            providerID = "test-provider",
            arguments = {
                name = "dont-use-me",
            },
        })
        assert_deep_eq(after_owned:to_split_table().data, {
            type = "plugin",
            name = "testing-after",
            pluginID = "test",
            providerID = "test-provider",
            arguments = {
                name = "not-used",
            },
        })

        assert(destroy_called == nil)
        before_owned:clear()
        assert(destroy_called ~= nil)
        destroy_called = nil

        assert(not get_name_called)
    end,

    same_channel = function()
        local chan1 = from_split_table({
            data = {
                type = "plugin",
                name = "chan1",
                pluginID = "test",
                providerID = "test-provider",
                arguments = {
                    name = "arg1",
                },
            },
        })
        assert(chan1:channel():get_type() == c2.ChannelType.Plugin)

        -- Different arguments, but should be deduplicated based on the name.
        local chan2 = from_split_table({
            data = {
                type = "plugin",
                name = "chan1",
                pluginID = "test",
                providerID = "test-provider",
                arguments = {
                    name = "arg2",
                },
            },
        })
        assert(chan2:channel():get_type() == c2.ChannelType.Plugin)
        assert(chan1:channel() == chan2:channel())

        c2.register_channel_provider({
            id = "test-provider",
            display_name = "display name",
            description = "just testing",
            arguments = {
                {
                    id = "name",
                    display_name = "Channel Name",
                    kind = "text",
                },
            },
            callbacks = {
                get_name = function(...)
                    error("don't call me")
                end,
                create = function(...)
                    return {}
                end,
            },
        })

        local chan3 = from_split_table({
            data = {
                type = "plugin",
                name = "chan1",
                pluginID = "test",
                providerID = "test-provider",
                arguments = {
                    name = "arg3",
                },
            },
        })
        assert(chan3:channel():get_type() == c2.ChannelType.Plugin)
        assert(chan1:channel() == chan3:channel())

        -- Different provider.
        local chan4 = from_split_table({
            data = {
                type = "plugin",
                name = "chan1",
                pluginID = "test",
                providerID = "test-provider2",
                arguments = {
                    name = "arg1",
                },
            },
        })
        assert(chan4:channel():get_type() == c2.ChannelType.Plugin)
        assert(chan4:channel() ~= chan1:channel())

        -- Different plugin.
        local chan5 = from_split_table({
            data = {
                type = "plugin",
                name = "chan1",
                pluginID = "test2",
                providerID = "test-provider",
                arguments = {
                    name = "arg1",
                },
            },
        })
        assert(chan5:channel():get_type() == c2.ChannelType.Plugin)
        assert(chan5:channel() ~= chan1:channel())
        assert(chan5:channel() ~= chan4:channel())
    end,
}

for name, fn in pairs(tests) do
    local ok, res = pcall(fn)

    for i, chan in pairs(_owned_channels) do
        chan:clear()
    end
    _owned_channels = {}
    c2test.clear_channel_providers()

    if not ok then
        error(name .. " failed: " .. res)
    end
end

-- Tell the language server about our globals.
---@diagnostic disable-next-line: unused-local
local function _meta()
    ---@diagnostic disable-next-line: lowercase-global
    c2test = {}
    ---@class c2test.OwnedChannel
    c2test.OwnedChannel = {}

    ---@param tbl table
    ---@return c2test.OwnedChannel
    function c2test.OwnedChannel.from_split_table(tbl)
        return {}
    end
    ---@return table
    function c2test.OwnedChannel:to_split_table()
        return {}
    end
    ---@return c2.Channel
    function c2test.OwnedChannel:channel()
        return {}
    end
    function c2test.OwnedChannel:clear() end
end
