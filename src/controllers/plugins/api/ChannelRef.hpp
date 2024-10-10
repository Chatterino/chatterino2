#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/Channel.hpp"
#    include "providers/twitch/TwitchChannel.hpp"

#    include <sol/forward.hpp>

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @includefile providers/twitch/TwitchChannel.hpp
 */

/**
 * @lua@class c2.Channel
 */
struct ChannelRef {
public:
    ChannelRef(const std::shared_ptr<Channel> &chan);

    /**
     * Returns true if the channel this object points to is valid.
     * If the object expired, returns false
     * If given a non-Channel object, it errors.
     *
     * @lua@return boolean success
     * @exposed c2.Channel:is_valid
     */
    bool is_valid();

    /**
     * Gets the channel's name. This is the lowercase login name.
     *
     * @lua@return string name
     * @exposed c2.Channel:get_name
     */
    QString get_name();

    /**
     * Gets the channel's type
     *
     * @lua@return c2.ChannelType
     * @exposed c2.Channel:get_type
     */
    Channel::Type get_type();

    /**
     * Get the channel owner's display name. This may contain non-lowercase ascii characters.
     *
     * @lua@return string name
     * @exposed c2.Channel:get_display_name
     */
    QString get_display_name();

    /**
     * Sends a message to the target channel.
     * Note that this does not execute client-commands.
     *
     * @lua@param message string
     * @lua@param execute_commands? boolean Should commands be run on the text?
     * @exposed c2.Channel:send_message
     */
    void send_message(QString text, sol::variadic_args va);

    /**
     * Adds a system message client-side
     *
     * @lua@param message string
     * @exposed c2.Channel:add_system_message
     */
    void add_system_message(QString text);

    /**
     * Returns true for twitch channels.
     * Compares the channel Type. Note that enum values aren't guaranteed, just
     * that they are equal to the exposed enum.
     *
     * @lua@return boolean
     * @exposed c2.Channel:is_twitch_channel
     */
    bool is_twitch_channel();

    /**
     * Twitch Channel specific functions
     */

    /**
     * Returns a copy of the channel mode settings (subscriber only, r9k etc.)
     *
     * @lua@return RoomModes
     * @exposed c2.Channel:get_room_modes
     */
    sol::table get_room_modes(sol::this_state state);

    /**
     * Returns a copy of the stream status.
     *
     * @lua@return StreamStatus
     * @exposed c2.Channel:get_stream_status
     */
    sol::table get_stream_status(sol::this_state state);

    /**
     * Returns the Twitch user ID of the owner of the channel.
     *
     * @lua@return string
     * @exposed c2.Channel:get_twitch_id
     */
    QString get_twitch_id();

    /**
     * Returns true if the channel is a Twitch channel and the user owns it
     *
     * @lua@return boolean
     * @exposed c2.Channel:is_broadcaster
     */
    bool is_broadcaster();

    /**
     * Returns true if the channel is a Twitch channel and the user is a moderator in the channel
     * Returns false for broadcaster.
     *
     * @lua@return boolean
     * @exposed c2.Channel:is_mod
     */
    bool is_mod();

    /**
     * Returns true if the channel is a Twitch channel and the user is a VIP in the channel
     * Returns false for broadcaster.
     *
     * @lua@return boolean
     * @exposed c2.Channel:is_vip
     */
    bool is_vip();

    /**
      * Misc
      */

    /**
     * @lua@return string
     * @exposed c2.Channel:__tostring
     */
    QString to_string();

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
     * @lua@return c2.Channel?
     * @exposed c2.Channel.by_name
     */
    static std::optional<ChannelRef> get_by_name(const QString &name);

    /**
     * Finds a channel by the Twitch user ID of its owner.
     *
     * @lua@param id string ID of the owner of the channel.
     * @lua@return c2.Channel?
     * @exposed c2.Channel.by_twitch_id
     */
    static std::optional<ChannelRef> get_by_twitch_id(const QString &id);

    static void createUserType(sol::table &c2);

private:
    std::weak_ptr<Channel> weak;

    /// Locks the weak pointer and throws if the pointer expired
    std::shared_ptr<Channel> strong();

    /// Locks the weak pointer and throws if the pointer is invalid
    std::shared_ptr<TwitchChannel> twitch();
};

// NOLINTEND(readability-identifier-naming)

sol::table toTable(lua_State *L, const TwitchChannel::RoomModes &modes);
sol::table toTable(lua_State *L, const TwitchChannel::StreamStatus &status);

}  // namespace chatterino::lua::api
#endif
