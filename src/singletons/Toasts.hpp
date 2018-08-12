#pragma once

#include "Application.hpp"
#include "common/Singleton.hpp"

//#include <mutex>

namespace chatterino {

enum class Platform : uint8_t;

class Toasts final : public Singleton
{
public:
    void sendChannelNotification(const QString &channelName, Platform p);
    /*
    Toasts();
    virtual void initialize(Settings &settings, Paths &paths) override final;
    */
    static bool isEnabled();

private:
    void sendWindowsNotification(const QString &channelName, Platform p);
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
