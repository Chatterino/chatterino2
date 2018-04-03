#include "providers/twitch/twitchhelpers.hpp"
#include "debug/log.hpp"

namespace chatterino {
namespace providers {
namespace twitch {

bool TrimChannelName(const QString &channelName, QString &outChannelName)
{
    if (channelName.length() < 3) {
        debug::Log("channel name length below 3");
        return false;
    }

    outChannelName = channelName.mid(1);

    return true;
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
