#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

//#include <mutex>

namespace chatterino {

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName, int &platform);
    bool isEnabled(const QString &channelName);
    /*
    Toasts();
    virtual void initialize(Settings &settings, Paths &paths) override final;
    */

private:
    void sendWindowsNotification(const QString &channelName, int &platform);
    /*
    void updateLiveChannels(const QString &channelName);
    void removeFromLiveChannels(const QString &channelName);

    bool wasChannelLive(const QString &channelName);

    void shouldChannelBeNotified(const QString &channelName);
    void sendChannelNotification(const QString &channelName);
    void sendWindowsNotification(const QString &channelName);
    std::vector<QString> liveChannels;
    std::mutex mutex_;
    */
};
}  // namespace chatterino
