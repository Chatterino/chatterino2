#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

#include <mutex>

namespace chatterino {

class Toasts final : public Singleton
{
public:
    Toasts();
    virtual void initialize(Settings &settings, Paths &paths) override final;
    void updateLiveChannels(const QString &channelName);
    void removeFromLiveChannels(const QString &channelName);

private:
    bool wasChannelLive(const QString &channelName);
    void sendChannelNotification(const QString &channelName);
    std::vector<QString> liveChannels;
    std::mutex mutex_;
};
}  // namespace chatterino
