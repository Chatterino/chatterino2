#pragma once

#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

class QDateTime;

namespace chatterino {
class TwitchChannel;
}  // namespace chatterino

namespace chatterino::eventsub {

void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Clear &action);

}  // namespace chatterino::eventsub
