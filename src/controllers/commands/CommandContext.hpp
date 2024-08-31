#pragma once

#include <QStringList>

#include <memory>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class TwitchChannel;

struct CommandContext {
    QStringList words;

    // Can be null
    ChannelPtr channel;

    // Can be null if `channel` is null or if `channel` is not a Twitch channel
    TwitchChannel *twitchChannel;
};

}  // namespace chatterino
