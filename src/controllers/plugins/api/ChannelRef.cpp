#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/ChannelRef.hpp"

#    include "Application.hpp"
#    include "common/Channel.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "providers/twitch/TwitchChannel.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"

#    include <sol/sol.hpp>

#    include <memory>
#    include <optional>

namespace chatterino::lua::api {

ChannelRef::ChannelRef(const std::shared_ptr<Channel> &chan)
    : weak(chan)
{
}

std::shared_ptr<Channel> ChannelRef::strong()
{
    auto c = this->weak.lock();
    if (!c)
    {
        throw std::runtime_error(
            "Expired c2.Channel used - use c2.Channel:is_valid() to "
            "check validity");
    }
    return c;
}

std::shared_ptr<TwitchChannel> ChannelRef::twitch()
{
    auto c = std::dynamic_pointer_cast<TwitchChannel>(this->weak.lock());
    if (!c)
    {
        throw std::runtime_error(
            "Expired or non-twitch c2.Channel used - use "
            "c2.Channel:is_valid() and c2.Channe:is_twitch_channel()");
    }
    return c;
}

bool ChannelRef::is_valid()
{
    return !this->weak.expired();
}

QString ChannelRef::get_name()
{
    return this->strong()->getName();
}

Channel::Type ChannelRef::get_type()
{
    return this->strong()->getType();
}

QString ChannelRef::get_display_name()
{
    return this->strong()->getDisplayName();
}

void ChannelRef::send_message(QString text, sol::variadic_args va)
{
    bool execCommands = [&] {
        if (va.size() >= 1)
        {
            return va.get<bool>();
        }
        return false;
    }();
    text = text.replace('\n', ' ');
    auto chan = this->strong();
    if (execCommands)
    {
        text = getApp()->getCommands()->execCommand(text, chan, false);
    }
    chan->sendMessage(text);
}

void ChannelRef::add_system_message(QString text)
{
    text = text.replace('\n', ' ');
    this->strong()->addSystemMessage(text);
}

bool ChannelRef::is_twitch_channel()
{
    return this->strong()->isTwitchChannel();
}

sol::table ChannelRef::get_room_modes(sol::this_state state)
{
    return toTable(state.L, *this->twitch()->accessRoomModes());
}

sol::table ChannelRef::get_stream_status(sol::this_state state)
{
    return toTable(state.L, *this->twitch()->accessStreamStatus());
}

QString ChannelRef::get_twitch_id()
{
    return this->twitch()->roomId();
}

bool ChannelRef::is_broadcaster()
{
    return this->twitch()->isBroadcaster();
}

bool ChannelRef::is_mod()
{
    return this->twitch()->isMod();
}

bool ChannelRef::is_vip()
{
    return this->twitch()->isVip();
}

QString ChannelRef::to_string()
{
    auto chan = this->weak.lock();
    if (!chan)
    {
        return "<c2.Channel expired>";
    }
    return QStringView(u"<c2.Channel %1>").arg(chan->getName());
}

std::optional<ChannelRef> ChannelRef::get_by_name(const QString &name)
{
    auto chan = getApp()->getTwitch()->getChannelOrEmpty(name);
    if (chan->isEmpty())
    {
        return std::nullopt;
    }
    return chan;
}

std::optional<ChannelRef> ChannelRef::get_by_twitch_id(const QString &id)
{
    auto chan = getApp()->getTwitch()->getChannelOrEmptyByID(id);
    if (chan->isEmpty())
    {
        return std::nullopt;
    }
    return chan;
}

void ChannelRef::createUserType(sol::table &c2)
{
    // clang-format off
    c2.new_usertype<ChannelRef>(
        "Channel", sol::no_constructor, 
        // meta methods
        sol::meta_method::to_string, &ChannelRef::to_string,

        // Channel
        "is_valid", &ChannelRef::is_valid,
        "get_name",&ChannelRef::get_name,
        "get_type", &ChannelRef::get_type,
        "get_display_name", &ChannelRef::get_display_name,
        "send_message", &ChannelRef::send_message,
        "add_system_message", &ChannelRef::add_system_message,
        "is_twitch_channel", &ChannelRef::is_twitch_channel,

        // TwitchChannel
        "get_room_modes", &ChannelRef::get_room_modes, 
        "get_stream_status", &ChannelRef::get_stream_status,
        "get_twitch_id", &ChannelRef::get_twitch_id,
        "is_broadcaster", &ChannelRef::is_broadcaster,
        "is_mod", &ChannelRef::is_mod,
        "is_vip", &ChannelRef::is_vip,

        // static
        "by_name", &ChannelRef::get_by_name,
        "by_twitch_id", &ChannelRef::get_by_twitch_id
    );
    // clang-format on
}

sol::table toTable(lua_State *L, const TwitchChannel::RoomModes &modes)
{
    auto maybe = [](int value) {
        if (value >= 0)
        {
            return std::optional{value};
        }
        return std::optional<int>{};
    };
    // clang-format off
    return sol::table::create_with(L,
        "subscriber_only", modes.submode,
        "unique_chat", modes.r9k,
        "emotes_only", modes.emoteOnly,
        "follower_only", maybe(modes.followerOnly),
        "slow_mode", maybe(modes.slowMode)
    );
    // clang-format on
}

sol::table toTable(lua_State *L, const TwitchChannel::StreamStatus &status)
{
    // clang-format off
    return sol::table::create_with(L,
        "live", status.live,
        "viewer_count", status.viewerCount,
        "title", status.title,
        "game_name", status.game,
        "game_id", status.gameId,
        "uptime", status.uptimeSeconds
    );
    // clang-format on
}

}  // namespace chatterino::lua::api

#endif
