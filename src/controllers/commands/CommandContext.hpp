#pragma once

#include "common/Channel.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QStringList>

namespace chatterino {

struct CommandContext {
    QStringList words;

    // Can be null
    ChannelPtr channel;

    // Can be null if `channel` is null or if `channel` is not a Twitch channel
    TwitchChannel *twitchChannel;
};

}  // namespace chatterino
