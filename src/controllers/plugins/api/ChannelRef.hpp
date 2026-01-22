// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/Channel.hpp"
#    include "providers/twitch/TwitchChannel.hpp"

#    include <sol/forward.hpp>

namespace chatterino::lua {
class ThisPluginState;
}  // namespace chatterino::lua

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

struct ConnectionHandle;

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
     * Adds a message client-side
     *
     * @lua@param message c2.Message
     * @lua@param context? c2.MessageContext The context of the message being added
     * @lua@param override_flags? c2.MessageFlag|nil Flags to override the message's flags (some splits might filter for this)
     * @exposed c2.Channel:add_message
     */
    void add_message(std::shared_ptr<Message> &message, sol::variadic_args va);

    // FIXME: create a separate type for Sol container wrappers
    /**
     * Get a list of messages in this channel (starting from the most recent messages).
     * The snapshot is returned as a usertype that wraps a C++ object.
     *
     * @lua@param n_items number Number of messages to retrieve. This is an upper bound, the actual number of messages returned might be lower.
     * @lua@return c2.Message[]
     * @exposed c2.Channel:message_snapshot
     */
    std::vector<MessagePtrMut> message_snapshot(size_t n_items);

    /**
     * Get the most recent message. If this channel doesn't have any message, this returns `nil`.
     * 
     * @lua@return c2.Message?
     * @exposed c2.Channel:last_message
     */
    MessagePtrMut last_message();

    /**
     * Replace a specific message with a different one.
     * 
     * @lua@param message c2.Message The message to replace.
     * @lua@param replacement c2.Message The replacement.
     * @exposed c2.Channel:replace_message
     */
    void replace_message(const MessagePtrMut &message,
                         const MessagePtrMut &replacement);
    /**
     * Replace a specific message with a different one.
     * 
     * @lua@param message c2.Message The message to replace.
     * @lua@param replacement c2.Message The replacement.
     * @lua@param hint number A one-based index (from the start) where the message is probably located. This is checked first. Otherwise the behavior is identical to the overload without this parameter.
     * @exposed c2.Channel:replace_message
     */
    void replace_message_hint(const MessagePtrMut &message,
                              const MessagePtrMut &replacement, size_t hint);

    /**
     * Replace a message at an index with a different one.
     * 
     * @lua@param index number A one-based index (from the start) of the message to replace.
     * @lua@param replacement c2.Message The replacement.
     * @exposed c2.Channel:replace_message_at
     */
    void replace_message_at(size_t index, const MessagePtrMut &replacement);

    /**
     * Remove all messages in this channel.
     *
     * @exposed c2.Channel:clear_messages
     */
    void clear_messages();

    /**
     * Find a message by its ID.
     *
     * @lua@param id string
     * @lua@return c2.Message?
     * @exposed c2.Channel:find_message_by_id
     */
    MessagePtrMut find_message_by_id(const QString &id);

    /**
     * Check if the channel has any messages.
     *
     * @lua@return boolean
     * @exposed c2.Channel:has_messages
     */
    bool has_messages();

    /**
     * Count the number of messages in this channel.
     *
     * @lua@return number
     * @exposed c2.Channel:count_messages
     */
    size_t count_messages();

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

    bool operator==(const ChannelRef &other) const noexcept;

    /**
     * Callback when the channel display name changes.
     *
     * @lua@param cb fun()
     * @lua@return c2.ConnectionHandle hdl
     * @exposed c2.Channel:on_display_name_changed
     */
    ConnectionHandle on_display_name_changed(ThisPluginState state,
                                             sol::main_protected_function pfn);

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
