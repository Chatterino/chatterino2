---@meta Chatterino2

-- This file is automatically generated from src/controllers/plugins/LuaAPI.hpp by the scripts/make_luals_meta.py script
-- This file is intended to be used with LuaLS (https://luals.github.io/).
-- Add the folder this file is in to "Lua.workspace.library".

c2 = {}
---@alias c2.LogLevel integer
---@type { Debug: c2.LogLevel, Info: c2.LogLevel, Warning: c2.LogLevel, Critical: c2.LogLevel }
c2.LogLevel = {}

---@alias c2.EventType integer
---@type { CompletionRequested: c2.EventType }
c2.EventType = {}

---@class CommandContext
---@field words string[] The words typed when executing the command. For example `/foo bar baz` will result in `{"/foo", "bar", "baz"}`.
---@field channel c2.Channel The channel the command was executed in.

---@class CompletionList
---@field values string[] The completions
---@field hide_others boolean Whether other completions from Chatterino should be hidden/ignored.

---@class CompletionEvent
---@field query string The word being completed
---@field full_text_content string Content of the text input
---@field cursor_position integer Position of the cursor in the text input in unicode codepoints (not bytes)
---@field is_first_word boolean True if this is the first word in the input

-- Begin src/common/Channel.hpp

---@alias c2.ChannelType integer
---@type { None: c2.ChannelType, Direct: c2.ChannelType, Twitch: c2.ChannelType, TwitchWhispers: c2.ChannelType, TwitchWatching: c2.ChannelType, TwitchMentions: c2.ChannelType, TwitchLive: c2.ChannelType, TwitchAutomod: c2.ChannelType, TwitchEnd: c2.ChannelType, Misc: c2.ChannelType }
c2.ChannelType = {}

-- End src/common/Channel.hpp

-- Begin src/controllers/plugins/api/ChannelRef.hpp

-- Begin src/providers/twitch/TwitchChannel.hpp

---@class StreamStatus
---@field live boolean
---@field viewer_count number
---@field title string Stream title or last stream title
---@field game_name string
---@field game_id string
---@field uptime number Seconds since the stream started.

---@class RoomModes
---@field subscriber_only boolean
---@field unique_chat boolean You might know this as r9kbeta or robot9000.
---@field emotes_only boolean Whether or not text is allowed in messages. Note that "emotes" here only means Twitch emotes, not Unicode emoji, nor 3rd party text-based emotes

-- End src/providers/twitch/TwitchChannel.hpp

---@class c2.Channel
c2.Channel = {}

--- Returns true if the channel this object points to is valid.
--- If the object expired, returns false
--- If given a non-Channel object, it errors.
---
---@return boolean success
function c2.Channel:is_valid() end

--- Gets the channel's name. This is the lowercase login name.
---
---@return string name
function c2.Channel:get_name() end

--- Gets the channel's type
---
---@return c2.ChannelType
function c2.Channel:get_type() end

--- Get the channel owner's display name. This may contain non-lowercase ascii characters.
---
---@return string name
function c2.Channel:get_display_name() end

--- Sends a message to the target channel.
--- Note that this does not execute client-commands.
---
---@param message string
---@param execute_commands? boolean Should commands be run on the text?
function c2.Channel:send_message(message, execute_commands) end

--- Adds a system message client-side
---
---@param message string
function c2.Channel:add_system_message(message) end

--- Returns true for twitch channels.
--- Compares the channel Type. Note that enum values aren't guaranteed, just
--- that they are equal to the exposed enum.
---
---@return boolean
function c2.Channel:is_twitch_channel() end

--- Returns a copy of the channel mode settings (subscriber only, r9k etc.)
---
---@return RoomModes
function c2.Channel:get_room_modes() end

--- Returns a copy of the stream status.
---
---@return StreamStatus
function c2.Channel:get_stream_status() end

--- Returns the Twitch user ID of the owner of the channel.
---
---@return string
function c2.Channel:get_twitch_id() end

--- Returns true if the channel is a Twitch channel and the user owns it
---
---@return boolean
function c2.Channel:is_broadcaster() end

--- Returns true if the channel is a Twitch channel and the user is a moderator in the channel
--- Returns false for broadcaster.
---
---@return boolean
function c2.Channel:is_mod() end

--- Returns true if the channel is a Twitch channel and the user is a VIP in the channel
--- Returns false for broadcaster.
---
---@return boolean
function c2.Channel:is_vip() end

---@return string
function c2.Channel:__tostring() end

--- Finds a channel by name.
--- Misc channels are marked as Twitch:
--- - /whispers
--- - /mentions
--- - /watching
--- - /live
--- - /automod
---
---@param name string Which channel are you looking for?
---@return c2.Channel?
function c2.Channel.by_name(name) end

--- Finds a channel by the Twitch user ID of its owner.
---
---@param id string ID of the owner of the channel.
---@return c2.Channel?
function c2.Channel.by_twitch_id(id) end

-- End src/controllers/plugins/api/ChannelRef.hpp

-- Begin src/controllers/plugins/api/HTTPResponse.hpp

---@class HTTPResponse
HTTPResponse = {}

--- Returns the data. This is not guaranteed to be encoded using any
--- particular encoding scheme. It's just the bytes the server returned.
---
function HTTPResponse:data() end

--- Returns the status code.
---
function HTTPResponse:status() end

--- A somewhat human readable description of an error if such happened
---
function HTTPResponse:error() end

-- End src/controllers/plugins/api/HTTPResponse.hpp

-- Begin src/controllers/plugins/api/HTTPRequest.hpp

---@alias HTTPCallback fun(result: HTTPResponse): nil
---@class HTTPRequest
HTTPRequest = {}

--- Sets the success callback
---
---@param callback HTTPCallback Function to call when the HTTP request succeeds
function HTTPRequest:on_success(callback) end

--- Sets the failure callback
---
---@param callback HTTPCallback Function to call when the HTTP request fails or returns a non-ok status
function HTTPRequest:on_error(callback) end

--- Sets the finally callback
---
---@param callback fun(): nil Function to call when the HTTP request finishes
function HTTPRequest:finally(callback) end

--- Sets the timeout
---
---@param timeout integer How long in milliseconds until the times out
function HTTPRequest:set_timeout(timeout) end

--- Sets the request payload
---
---@param data string
function HTTPRequest:set_payload(data) end

--- Sets a header in the request
---
---@param name string
---@param value string
function HTTPRequest:set_header(name, value) end

--- Executes the HTTP request
---
function HTTPRequest:execute() end

--- Creates a new HTTPRequest
---
---@param method HTTPMethod Method to use
---@param url string Where to send the request to
---@return HTTPRequest
function HTTPRequest.create(method, url) end

-- End src/controllers/plugins/api/HTTPRequest.hpp

-- Begin src/common/network/NetworkCommon.hpp

---@alias HTTPMethod integer
---@type { Get: HTTPMethod, Post: HTTPMethod, Put: HTTPMethod, Delete: HTTPMethod, Patch: HTTPMethod }
HTTPMethod = {}

-- End src/common/network/NetworkCommon.hpp

--- Registers a new command called `name` which when executed will call `handler`.
---
---@param name string The name of the command.
---@param handler fun(ctx: CommandContext) The handler to be invoked when the command gets executed.
---@return boolean ok  Returns `true` if everything went ok, `false` if a command with this name exists.
function c2.register_command(name, handler) end

--- Registers a callback to be invoked when completions for a term are requested.
---
---@param type "CompletionRequested"
---@param func fun(event: CompletionEvent): CompletionList The callback to be invoked.
function c2.register_callback(type, func) end

--- Writes a message to the Chatterino log.
---
---@param level c2.LogLevel The desired level.
---@param ... any Values to log. Should be convertible to a string with `tostring()`.
function c2.log(level, ...) end

--- Calls callback around msec milliseconds later. Does not freeze Chatterino.
---
---@param callback fun() The callback that will be called.
---@param msec number How long to wait.
function c2.later(callback, msec) end

