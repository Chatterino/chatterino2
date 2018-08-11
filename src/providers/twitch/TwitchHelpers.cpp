#include "providers/twitch/TwitchHelpers.hpp"
#include "debug/Log.hpp"

namespace chatterino {

bool trimChannelName(const QString &channelName, QString &outChannelName)
{
    if (channelName.length() < 3) {
        log("channel name length below 3");
        return false;
    }

    outChannelName = channelName.mid(1);

    return true;
}

}  // namespace chatterino
