#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/Channel.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "providers/twitch/TwitchChannel.hpp"

#    include <optional>

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * This enum describes a platform for the purpose of searching for a channel.
 * Currently only Twitch is supported because identifying IRC channels is tricky.
 * @exposeenum Platform
 */
enum class LPlatform {
    Twitch,
    //IRC,
};

/**
 * @lua@class Channel
 */
struct ChannelRef {
    static void createMetatable(lua_State *L);
    friend class chatterino::PluginController;

    /**
     * @brief Get the content of the top object on Lua stack, usually first argument to function as a ChannelPtr.
     * If the object given is not a userdatum or the pointer inside that
     * userdatum doesn't point to a Channel, a lua error is thrown.
     *
     * @param expiredOk Should an expired return nullptr instead of erroring
     */
    static ChannelPtr getOrError(lua_State *L, bool expiredOk = false);

    /**
     * @brief Casts the result of getOrError to std::shared_ptr<TwitchChannel>
     * if that fails thows a lua error.
     */
    static std::shared_ptr<TwitchChannel> getTwitchOrError(lua_State *L);

public:
    /**
     * Returns true if the channel this object points to is valid.
     * If the object expired, returns false
     * If given a non-Channel object, it errors.
     *
     * @lua@return boolean success
     * @exposed Channel:is_valid
     */
    static int is_valid(lua_State *L);

    /**
     * Gets the channel's name. This is the lowercase login name.
     *
     * @lua@return string name
     * @exposed Channel:get_name
     */
    static int get_name(lua_State *L);

    /**
     * Gets the channel's type
     *
     * @lua@return ChannelType
     * @exposed Channel:get_type
     */
    static int get_type(lua_State *L);

    /**
     * Get the channel owner's display name. This may contain non-lowercase ascii characters.
     *
     * @lua@return string name
     * @exposed Channel:get_display_name
     */
    static int get_display_name(lua_State *L);

    /**
     * Sends a message to the target channel.
     * Note that this does not execute client-commands.
     *
     * @lua@param message string
     * @lua@param execute_commands boolean Should commands be run on the text?
     * @exposed Channel:send_message
     */
    static int send_message(lua_State *L);

    /**
     * Adds a system message client-side
     *
     * @lua@param message string
     * @exposed Channel:add_system_message
     */
    static int add_system_message(lua_State *L);

    /**
     * Returns true for twitch channels.
     * Compares the channel Type. Note that enum values aren't guaranteed, just
     * that they are equal to the exposed enum.
     *
     * @lua@return boolean
     * @exposed Channel:is_twitch_channel
     */
    static int is_twitch_channel(lua_State *L);

    /**
     * Twitch Channel specific functions
     */

    /**
     * Returns a copy of the channel mode settings (subscriber only, r9k etc.)
     *
     * @lua@return RoomModes
     * @exposed Channel:get_room_modes
     */
    static int get_room_modes(lua_State *L);

    /**
     * Returns a copy of the stream status.
     *
     * @lua@return StreamStatus
     * @exposed Channel:get_stream_status
     */
    static int get_stream_status(lua_State *L);

    /**
     * Returns the Twitch user ID of the owner of the channel.
     *
     * @lua@return string
     * @exposed Channel:get_twitch_id
     */
    static int get_twitch_id(lua_State *L);

    /**
     * Returns true if the channel is a Twitch channel and the user owns it
     *
     * @lua@return boolean
     * @exposed Channel:is_broadcaster
     */
    static int is_broadcaster(lua_State *L);

    /**
     * Returns true if the channel is a Twitch channel and the user is a moderator in the channel
     * Returns false for broadcaster.
     *
     * @lua@return boolean
     * @exposed Channel:is_mod
     */
    static int is_mod(lua_State *L);

    /**
     * Returns true if the channel is a Twitch channel and the user is a VIP in the channel
     * Returns false for broadcaster.
     *
     * @lua@return boolean
     * @exposed Channel:is_vip
     */
    static int is_vip(lua_State *L);

    /**
      * Misc
      */

    /**
     * @lua@return string
     * @exposed Channel:__tostring
     */
    static int to_string(lua_State *L);

    /**
     * Static functions
     */

    /**
     * Finds a channel by name.
     *
     * Misc channels are marked as Twitch:
     *  - /whispers
     *  - /mentions
     *  - /watching
     *  - /live
     *  - /automod
     *
     * @lua@param name string Which channel are you looking for?
     * @lua@param platform Platform Where to search for the channel?
     * @lua@return Channel?
     * @exposed Channel.by_name
     */
    static int get_by_name(lua_State *L);

    /**
     * Finds a channel by the Twitch user ID of its owner.
     *
     * @lua@param id string ID of the owner of the channel.
     * @lua@return Channel?
     * @exposed Channel.by_twitch_id
     */
    static int get_by_twitch_id(lua_State *L);
};

// This is a copy of the TwitchChannel::RoomModes structure, except it uses nicer optionals
/**
 * @lua@class RoomModes
 */
struct LuaRoomModes {
    /**
     * @lua@field unique_chat boolean You might know this as r9kbeta or robot9000.
     */
    bool unique_chat = false;

    /**
     * @lua@field subscriber_only boolean
     */
    bool subscriber_only = false;

    /**
     * @lua@field emotes_only boolean Whether or not text is allowed in messages. Note that "emotes" here only means Twitch emotes, not Unicode emoji, nor 3rd party text-based emotes
     */
    bool emotes_only = false;

    /**
     * @lua@field follower_only number? Time in minutes you need to follow to chat or nil.
     */
    std::optional<int> follower_only;
    /**
     * @lua@field slow_mode number? Time in seconds you need to wait before sending messages or nil.
     */
    std::optional<int> slow_mode;
};

/**
 * @lua@class StreamStatus
 */
struct LuaStreamStatus {
    /**
     * @lua@field live boolean
     */
    bool live = false;

    /**
     * @lua@field viewer_count number
     */
    int viewer_count = 0;

    /**
     * @lua@field uptime number Seconds since the stream started.
     */
    int uptime = 0;

    /**
     * @lua@field title string Stream title or last stream title
     */
    QString title;

    /**
     * @lua@field game_name string
     */
    QString game_name;

    /**
     * @lua@field game_id string
     */
    QString game_id;
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
namespace chatterino::lua {
StackIdx push(lua_State *L, const api::LuaRoomModes &modes);
StackIdx push(lua_State *L, const api::LuaStreamStatus &status);
StackIdx push(lua_State *L, ChannelPtr chn);
}  // namespace chatterino::lua
#endif
