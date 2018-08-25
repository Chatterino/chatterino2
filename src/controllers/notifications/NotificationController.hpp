#pragma once

#include "common/SignalVector.hpp"
#include "singletons/Settings.hpp"

#include <QTimer>

namespace chatterino {

class Settings;
class Paths;

class NotificationModel;

enum class Platform : uint8_t {
    Twitch,  // 0
    // Mixer,   // 1
};

class NotificationController final : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isChannelNotified(const QString &channelName, Platform p);
    void updateChannelNotification(const QString &channelName, Platform p);
    void addChannelNotification(const QString &channelName, Platform p);
    void removeChannelNotification(const QString &channelName, Platform p);

    void playSound();

    UnsortedSignalVector<QString> getVector(Platform p);

    std::map<Platform, UnsortedSignalVector<QString>> channelMap;

    NotificationModel *createModel(QObject *parent, Platform p);

private:
    bool initialized_ = false;
    QTimer *liveStatusTimer_;
    void removeFakeChannel(const QString channelName);
    std::vector<QString> fakeTwitchChannels;
    void getFakeTwitchChannelLiveStatus(const QString &channelName);

    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
    /*
    ChatterinoSetting<std::vector<QString>> mixerSetting_ = {
        "/notifications/mixer"};
    */
private slots:
    void fetchFakeChannels();
};

}  // namespace chatterino
