#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/ChannelRef.hpp"

#    include "common/Channel.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchChannel.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"

#    include <lauxlib.h>
#    include <lua.h>

#    include <cassert>
#    include <memory>
#    include <optional>

namespace chatterino::lua::api {
// NOLINTBEGIN(*vararg)

// NOLINTNEXTLINE(*-avoid-c-arrays)
static const luaL_Reg CHANNEL_REF_METHODS[] = {
    {"is_valid", &ChannelRef::is_valid},
    {"get_name", &ChannelRef::get_name},
    {"get_type", &ChannelRef::get_type},
    {"get_display_name", &ChannelRef::get_display_name},
    {"send_message", &ChannelRef::send_message},
    {"add_system_message", &ChannelRef::add_system_message},
    {"is_twitch_channel", &ChannelRef::is_twitch_channel},

    // Twitch
    {"get_room_modes", &ChannelRef::get_room_modes},
    {"get_stream_status", &ChannelRef::get_stream_status},
    {"get_twitch_id", &ChannelRef::get_twitch_id},
    {"is_broadcaster", &ChannelRef::is_broadcaster},
    {"is_mod", &ChannelRef::is_mod},
    {"is_vip", &ChannelRef::is_vip},

    // misc
    {"__tostring", &ChannelRef::to_string},

    // static
    {"by_name", &ChannelRef::get_by_name},
    {"by_twitch_id", &ChannelRef::get_by_twitch_id},
    {nullptr, nullptr},
};

void ChannelRef::createMetatable(lua_State *L)
{
    lua::StackGuard guard(L, 1);

    luaL_newmetatable(L, "c2.Channel");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  // clone metatable
    lua_settable(L, -3);   // metatable.__index = metatable

    // Generic IWeakResource stuff
    lua_pushstring(L, "__gc");
    lua_pushcfunction(
        L, (&WeakPtrUserData<UserData::Type::Channel, ChannelRef>::destroy));
    lua_settable(L, -3);  // metatable.__gc = WeakPtrUserData<...>::destroy

    luaL_setfuncs(L, CHANNEL_REF_METHODS, 0);
}

ChannelPtr ChannelRef::getOrError(lua_State *L, bool expiredOk)
{
    if (lua_gettop(L) < 1)
    {
        luaL_error(L, "Called c2.Channel method without a channel object");
        return nullptr;
    }
    if (lua_isuserdata(L, lua_gettop(L)) == 0)
    {
        luaL_error(
            L, "Called c2.Channel method with a non-userdata 'self' argument");
        return nullptr;
    }
    // luaL_checkudata is no-return if check fails
    auto *checked = luaL_checkudata(L, lua_gettop(L), "c2.Channel");
    auto *data =
        WeakPtrUserData<UserData::Type::Channel, Channel>::from(checked);
    if (data == nullptr)
    {
        luaL_error(L,
                   "Called c2.Channel method with an invalid channel pointer");
        return nullptr;
    }
    lua_pop(L, 1);
    if (data->target.expired())
    {
        if (!expiredOk)
        {
            luaL_error(L,
                       "Usage of expired c2.Channel object. Underlying "
                       "resource was freed. Use Channel:is_valid() to check");
        }
        return nullptr;
    }
    return data->target.lock();
}

std::shared_ptr<TwitchChannel> ChannelRef::getTwitchOrError(lua_State *L)
{
    auto ref = ChannelRef::getOrError(L);
    auto ptr = dynamic_pointer_cast<TwitchChannel>(ref);
    if (ptr == nullptr)
    {
        luaL_error(L,
                   "c2.Channel Twitch-only operation on non-Twitch channel.");
    }
    return ptr;
}

int ChannelRef::is_valid(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L, true);
    lua::push(L, that != nullptr);
    return 1;
}

int ChannelRef::get_name(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L);
    lua::push(L, that->getName());
    return 1;
}

int ChannelRef::get_type(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L);
    lua::push(L, that->getType());
    return 1;
}

int ChannelRef::get_display_name(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L);
    lua::push(L, that->getDisplayName());
    return 1;
}

int ChannelRef::send_message(lua_State *L)
{
    if (lua_gettop(L) != 2 && lua_gettop(L) != 3)
    {
        luaL_error(L, "Channel:send_message needs 1 or 2 arguments (message "
                      "text and optionally execute_commands flag)");
        return 0;
    }
    bool execcmds = false;
    if (lua_gettop(L) == 3)
    {
        if (!lua::pop(L, &execcmds))
        {
            luaL_error(L, "cannot get execute_commands (2nd argument of "
                          "Channel:send_message)");
            return 0;
        }
    }

    QString text;
    if (!lua::pop(L, &text))
    {
        luaL_error(L, "cannot get text (1st argument of Channel:send_message)");
        return 0;
    }

    ChannelPtr that = ChannelRef::getOrError(L);

    text = text.replace('\n', ' ');
    if (execcmds)
    {
        text = getIApp()->getCommands()->execCommand(text, that, false);
    }
    that->sendMessage(text);
    return 0;
}

int ChannelRef::add_system_message(lua_State *L)
{
    // needs to account for the hidden self argument
    if (lua_gettop(L) != 2)
    {
        luaL_error(
            L, "Channel:add_system_message needs exactly 1 argument (message "
               "text)");
        return 0;
    }

    QString text;
    if (!lua::pop(L, &text))
    {
        luaL_error(
            L, "cannot get text (1st argument of Channel:add_system_message)");
        return 0;
    }
    ChannelPtr that = ChannelRef::getOrError(L);
    text = text.replace('\n', ' ');
    that->addMessage(makeSystemMessage(text));
    return 0;
}

int ChannelRef::is_twitch_channel(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L);
    lua::push(L, that->isTwitchChannel());
    return 1;
}

int ChannelRef::get_room_modes(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    const auto m = tc->accessRoomModes();
    const auto modes = LuaRoomModes{
        .unique_chat = m->r9k,
        .subscriber_only = m->submode,
        .emotes_only = m->emoteOnly,
        .follower_only = (m->followerOnly == -1)
                             ? std::nullopt
                             : std::optional(m->followerOnly),
        .slow_mode =
            (m->slowMode == 0) ? std::nullopt : std::optional(m->slowMode),

    };
    lua::push(L, modes);
    return 1;
}

int ChannelRef::get_stream_status(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    const auto s = tc->accessStreamStatus();
    const auto status = LuaStreamStatus{
        .live = s->live,
        .viewer_count = static_cast<int>(s->viewerCount),
        .uptime = s->uptimeSeconds,
        .title = s->title,
        .game_name = s->game,
        .game_id = s->gameId,
    };
    lua::push(L, status);
    return 1;
}

int ChannelRef::get_twitch_id(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    lua::push(L, tc->roomId());
    return 1;
}

int ChannelRef::is_broadcaster(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    lua::push(L, tc->isBroadcaster());
    return 1;
}

int ChannelRef::is_mod(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    lua::push(L, tc->isMod());
    return 1;
}

int ChannelRef::is_vip(lua_State *L)
{
    auto tc = ChannelRef::getTwitchOrError(L);
    lua::push(L, tc->isVip());
    return 1;
}

int ChannelRef::get_by_name(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "Channel.by_name needs exactly 2 arguments (channel "
                      "name and platform)");
        lua_pushnil(L);
        return 1;
    }
    LPlatform platform{};
    if (!lua::pop(L, &platform))
    {
        luaL_error(L, "cannot get platform (2nd argument of Channel.by_name, "
                      "expected a string)");
        lua_pushnil(L);
        return 1;
    }
    QString name;
    if (!lua::pop(L, &name))
    {
        luaL_error(L,
                   "cannot get channel name (1st argument of Channel.by_name, "
                   "expected a string)");
        lua_pushnil(L);
        return 1;
    }
    auto chn = getApp()->twitch->getChannelOrEmpty(name);
    lua::push(L, chn);
    return 1;
}

int ChannelRef::get_by_twitch_id(lua_State *L)
{
    if (lua_gettop(L) != 1)
    {
        luaL_error(
            L, "Channel.by_twitch_id needs exactly 1 arguments (channel owner "
               "id)");
        lua_pushnil(L);
        return 1;
    }
    QString id;
    if (!lua::pop(L, &id))
    {
        luaL_error(L,
                   "cannot get channel name (1st argument of Channel.by_name, "
                   "expected a string)");
        lua_pushnil(L);
        return 1;
    }
    auto chn = getApp()->twitch->getChannelOrEmptyByID(id);

    lua::push(L, chn);
    return 1;
}

int ChannelRef::to_string(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L, true);
    if (that == nullptr)
    {
        lua_pushstring(L, "<c2.Channel expired>");
        return 1;
    }
    QString formated = QString("<c2.Channel %1>").arg(that->getName());
    lua::push(L, formated);
    return 1;
}
}  // namespace chatterino::lua::api
// NOLINTEND(*vararg)
//
namespace chatterino::lua {
StackIdx push(lua_State *L, const api::LuaRoomModes &modes)
{
    auto out = lua::pushEmptyTable(L, 6);
#    define PUSH(field)            \
        lua::push(L, modes.field); \
        lua_setfield(L, out, #field)
    PUSH(unique_chat);
    PUSH(subscriber_only);
    PUSH(emotes_only);
    PUSH(follower_only);
    PUSH(slow_mode);
#    undef PUSH
    return out;
}

StackIdx push(lua_State *L, const api::LuaStreamStatus &status)
{
    auto out = lua::pushEmptyTable(L, 6);
#    define PUSH(field)             \
        lua::push(L, status.field); \
        lua_setfield(L, out, #field)
    PUSH(live);
    PUSH(viewer_count);
    PUSH(uptime);
    PUSH(title);
    PUSH(game_name);
    PUSH(game_id);
#    undef PUSH
    return out;
}

StackIdx push(lua_State *L, ChannelPtr chn)
{
    using namespace chatterino::lua::api;

    if (chn->isEmpty())
    {
        lua_pushnil(L);
        return lua_gettop(L);
    }
    WeakPtrUserData<UserData::Type::Channel, Channel>::create(
        L, chn->weak_from_this());
    luaL_getmetatable(L, "c2.Channel");
    lua_setmetatable(L, -2);
    return lua_gettop(L);
}

}  // namespace chatterino::lua
#endif
