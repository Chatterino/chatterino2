#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/Channel.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * This enum describes a platform for the purpose of searching for a channel.
 * @exposeenum Platform
 */
enum class LPlatform {
    Twitch,
    IRC,
};

/**
 * @lua@class Channel: IWeakResource
 */
struct ChannelRef {
    static void createMetatable(lua_State *L);
    friend class chatterino::PluginController;

    /**
     * Get the content of the top object on Lua stack, usually first argument
     * to function as a ChannelPtr.
     * If the object given is not a userdatum or the pointer inside that
     * userdatum doesn't point to a Channel, a lua error is thrown.
     *
     * @param expiredOk Should an expired return nullptr instead of erroring
     */
    static ChannelPtr getOrError(lua_State *L, bool expiredOk = false);

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
     * Returns true for twitch channels.
     * Compares the channel Type. Note that enum values aren't guaranteed, just
     * that they are equal to the exposed enum.
     *
     * @lua@return bool
     * @exposed Channel:is_twitch_channel
     */
    static int is_twitch_channel(lua_State *L);

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
     * @lua@param string id ID of the owner of the channel.
     * @lua@return Channel?
     * @exposed Channel.by_twitch_id
     */
    static int get_by_twitch_id(lua_State *L);

    /**
     * @lua@return string
     * @exposed Channel:__tostring
     */
    static int to_string(lua_State *L);
};

StackIdx push(lua_State *L, const ChannelRef &chn);
bool peek(lua_State *L, ChannelRef *chn, StackIdx idx = -1);

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
#endif
