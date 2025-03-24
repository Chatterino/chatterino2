#pragma once

#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

#include <memory>

class QDateTime;

namespace chatterino {

class TwitchChannel;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

}  // namespace chatterino

namespace chatterino::eventsub {

void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Clear &action);

/// <MODERATOR> timed out <USER> for <DURATION>[ in <CHANNEL>]: <REASON>
void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Timeout &action);

/// <MODERATOR> banned <USER>[ in <CHANNEL>]: <REASON>
void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Ban &action);

}  // namespace chatterino::eventsub
