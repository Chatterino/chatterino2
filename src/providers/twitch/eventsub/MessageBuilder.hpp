#pragma once

#include "messages/Message.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

#include <QDateTime>

namespace chatterino::eventsub {

/// <BROADCASTER> has added <USER> as a VIP of this channel.
MessagePtr makeVipMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Vip &action);

/// <BROADCASTER> has removed <USER> as a VIP of this channel.
MessagePtr makeUnvipMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unvip &action);

/// <MODERATOR> has warned <USER>: <REASON>
MessagePtr makeWarnMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Warn &action);

}  // namespace chatterino::eventsub
