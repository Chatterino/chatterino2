#include "providers/twitch/TwitchHelpers.hpp"

#include "common/QLogging.hpp"

namespace chatterino {

bool trimChannelName(const QString &channelName, QString &outChannelName)
{
    if (channelName.length() < 2)
    {
        qCDebug(chatterinoTwitch) << "channel name length below 2";
        return false;
    }

    outChannelName = channelName.mid(1);

    return true;
}

}  // namespace chatterino
