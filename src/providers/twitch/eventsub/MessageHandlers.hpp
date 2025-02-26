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

void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Timeout &action);

}  // namespace chatterino::eventsub
