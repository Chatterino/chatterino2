---@meta Chatterino2

-- This file is automatically generated from src/controllers/plugins/LuaAPI.hpp by the scripts/make_luals_meta.py script
-- This file is intended to be used with LuaLS (https://luals.github.io/).
-- Add the folder this file is in to "Lua.workspace.library".

c2 = {}

---@class IWeakResource

--- Returns true if the channel this object points to is valid.
--- If the object expired, returns false
--- If given a non-Channel object, it errors.
---@return boolean
function IWeakResource:is_valid() end


---@alias LogLevel integer
---@type { Debug: LogLevel, Info: LogLevel, Warning: LogLevel, Critical: LogLevel }
c2.LogLevel = {}

---@alias EventType integer
---@type { CompletionRequested: EventType }
c2.EventType = {}
---@class CommandContext
---@field words string[] The words typed when executing the command. For example `/foo bar baz` will result in `{"/foo", "bar", "baz"}`.
---@field channel Channel The channel the command was executed in.

---@class CompletionList
---@field values string[] The completions
---@field hide_others boolean Whether other completions from Chatterino should be hidden/ignored.
-- Now including data from src/common/Channel.hpp.

---@alias ChannelType integer
---@type { None: ChannelType }
ChannelType = {}
-- Back to src/controllers/plugins/LuaAPI.hpp.
-- Now including data from src/controllers/plugins/api/ChannelRef.hpp.
--- This enum describes a platform for the purpose of searching for a channel.
--- Currently only Twitch is supported because identifying IRC channels is tricky.

---@alias Platform integer
---@type { Twitch: Platform }
Platform = {}
---@class Channel: IWeakResource

--- Returns true if the channel this object points to is valid.
--- If the object expired, returns false
--- If given a non-Channel object, it errors.
---
---@return boolean success
function Channel:is_valid() end

--- Gets the channel's name. This is the lowercase login name.
---
---@return string name
function Channel:get_name() end

--- Gets the channel's type
---
---@return ChannelType
function Channel:get_type() end

--- Get the channel owner's display name. This may contain non-lowercase ascii characters.
---
---@return string name
function Channel:get_display_name() end

--- Sends a message to the target channel.
--- Note that this does not execute client-commands.
---
---@param message string
---@param execute_commands boolean Should commands be run on the text?
function Channel:send_message(message, execute_commands) end

--- Adds a system message client-side
---
---@param message string
function Channel:add_system_message(message) end

--- Returns true for twitch channels.
--- Compares the channel Type. Note that enum values aren't guaranteed, just
--- that they are equal to the exposed enum.
---
---@return bool
function Channel:is_twitch_channel() end

--- Twitch Channel specific functions

--- Returns a copy of the channel mode settings (subscriber only, r9k etc.)
---
---@return RoomModes
function Channel:get_room_modes() end

--- Returns a copy of the stream status.
---
---@return StreamStatus
function Channel:get_stream_status() end

--- Returns the Twitch user ID of the owner of the channel.
---
---@return string
function Channel:get_twitch_id() end

--- Returns true if the channel is a Twitch channel and the user owns it
---
---@return boolean
function Channel:is_broadcaster() end

--- Returns true if the channel is a Twitch channel and the user is a moderator in the channel
--- Returns false for broadcaster.
---
---@return boolean
function Channel:is_mod() end

--- Returns true if the channel is a Twitch channel and the user is a VIP in the channel
--- Returns false for broadcaster.
---
---@return boolean
function Channel:is_vip() end

--- Misc

---@return string
function Channel:__tostring() end

--- Static functions

--- Finds a channel by name.
---
--- Misc channels are marked as Twitch:
--- - /whispers
--- - /mentions
--- - /watching
--- - /live
--- - /automod
---
---@param name string Which channel are you looking for?
---@param platform Platform Where to search for the channel?
---@return Channel?
function Channel.by_name(name, platform) end

--- Finds a channel by the Twitch user ID of its owner.
---
---@param string id ID of the owner of the channel.
---@return Channel?
function Channel.by_twitch_id(string) end

---@class RoomModes
---@field unique_chat boolean You might know this as r9kbeta or robot9000.
---@field subscriber_only boolean
---@field emotes_only boolean Whether or not text is allowed in messages.

--- Note that "emotes" here only means Twitch emotes, not Unicode emoji, nor 3rd party text-based emotes

---@field unique_chat number? Time in minutes you need to follow to chat or nil.

---@field slow_mode number? Time in seconds you need to wait before sending messages or nil.

---@class StreamStatus
---@field live boolean
---@field viewer_count number
---@field uptime number Seconds since the stream started.
---@field title string Stream title or last stream title
---@field game_name string
---@field game_id string
-- Back to src/controllers/plugins/LuaAPI.hpp.

--- Registers a new command called `name` which when executed will call `handler`.
---
---@param name string The name of the command.
---@param handler fun(ctx: CommandContext) The handler to be invoked when the command gets executed.
---@return boolean ok  Returns `true` if everything went ok, `false` if a command with this name exists.
function c2.register_command(name, handler) end

--- Registers a callback to be invoked when completions for a term are requested.
---
---@param type "CompletionRequested"
---@param func fun(query: string, full_text_content: string, cursor_position: integer, is_first_word: boolean): CompletionList The callback to be invoked.
function c2.register_callback(type, func) end

--- Writes a message to the Chatterino log.
---
---@param level LogLevel The desired level.
---@param ... any Values to log. Should be convertible to a string with `tostring()`.
function c2.log(level, ...) end

