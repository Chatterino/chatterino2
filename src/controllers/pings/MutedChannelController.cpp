#include "controllers/pings/MutedChannelController.hpp"
#include "controllers/pings/MutedChannelModel.hpp"
#include "util/PersistSignalVector.hpp"

namespace chatterino {

void MutedChannelController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;

    persist(this->channels, "/pings/muted");
}

bool MutedChannelController::isMuted(const QString &channelName)
{
    for (const auto &channel : this->channels)
    {
        if (channelName.toLower() == channel.toLower())
        {
            return true;
        }
    }
    return false;
}

void MutedChannelController::mute(const QString &channelName)
{
    channels.append(channelName);
}

void MutedChannelController::unmute(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0; i != channels.raw().size(); i++)
    {
        if (channels.raw()[i].toLower() == channelName.toLower())
        {
            channels.removeAt(i);
            i--;
        }
    }
}

bool MutedChannelController::toggleMuted(const QString &channelName)
{
    if (this->isMuted(channelName))
    {
        unmute(channelName);
        return false;
    }
    else
    {
        mute(channelName);
        return true;
    }
}

}  // namespace chatterino
