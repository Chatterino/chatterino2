#include "Toasts.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"

namespace chatterino {

Toasts::Toasts()
{
}

void Toasts::initialize(Settings &settings, Paths &paths)
{
    getApp()->twitch2->forEachChannel([](ChannelPtr chn) {
        auto twchn = dynamic_cast<TwitchChannel *>(chn.get());
        twchn->liveStatusChanged.connect([twchn]() {
            const auto streamStatus = twchn->accessStreamStatus();
            if (streamStatus->live) {
                // to Live
                getApp()->toasts->updateLiveChannels(twchn->getName());
            } else {
                // to Offline
            }
        });
    });
}

void Toasts::updateLiveChannels(const QString &channelName)
{
    if (wasChannelLive(channelName)) {
        return;
    } else {
        std::lock_guard<std::mutex> lock(mutex_);
        liveChannels.push_back(channelName);
    }
}

bool Toasts::wasChannelLive(const QString &channelName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &str : liveChannels) {
        if (str == channelName) {
            return true;
        }
    }
    return false;
}
void Toasts::sendChannelNotification(const QString &channelName)
{
}

void Toasts::

}  // namespace chatterino
