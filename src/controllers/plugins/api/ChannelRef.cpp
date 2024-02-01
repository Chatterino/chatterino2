#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/ChannelRef.hpp"

#    include "common/Channel.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"

#    include <lauxlib.h>
#    include <lua.h>

namespace chatterino::lua::api {
// NOLINTBEGIN(*vararg)

// NOLINTNEXTLINE(*-avoid-c-arrays)
static const luaL_Reg CHANNEL_REF_METHODS[] = {
    {"is_valid", &ChannelRef::is_valid},
    {"get_name", &ChannelRef::get_name},
    {"get_type", &ChannelRef::get_type},
    {"get_display_name", &ChannelRef::get_display_name},
    {"is_twitch_channel", &ChannelRef::is_twitch_channel},
    {"send_message", &ChannelRef::send_message},
    {"add_system_message", &ChannelRef::add_system_message},

    {"__tostring", &ChannelRef::to_string},

    // static
    {"by_name", &ChannelRef::get_by_name},
    {nullptr, nullptr},
};

void ChannelRef::createMetatable(lua_State *L)
{
    lua::StackGuard guard(L, 1);

    luaL_newmetatable(L, "c2.Channel");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  // clone metatable
    lua_settable(L, -3);   // metatable.__index = metatable

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
            L, "Called c2.Channel method with a non Channel 'self' argument.");
        return nullptr;
    }
    auto *data = WeakPtrUserData<UserData::Type::Channel, Channel>::from(
        lua_touserdata(L, lua_gettop(L)));
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

int ChannelRef::is_twitch_channel(lua_State *L)
{
    ChannelPtr that = ChannelRef::getOrError(L);
    lua::push(L, that->isTwitchChannel());
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
    if (chn->isEmpty())
    {
        lua_pushnil(L);
        return 1;
    }
    // pushes onto stack
    WeakPtrUserData<UserData::Type::Channel, Channel>::create(
        L, chn->weak_from_this());
    luaL_getmetatable(L, "c2.Channel");
    lua_setmetatable(L, -2);
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
    if (chn->isEmpty())
    {
        lua_pushnil(L);
        return 1;
    }
    // pushes onto stack
    WeakPtrUserData<UserData::Type::Channel, Channel>::create(
        L, chn->weak_from_this());
    luaL_getmetatable(L, "c2.Channel");
    lua_setmetatable(L, -2);
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

// NOLINTEND(*vararg)
}  // namespace chatterino::lua::api
#endif
