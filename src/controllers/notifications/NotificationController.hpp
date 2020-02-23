#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/Settings.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"

#include <QTimer>

namespace chatterino {

class Settings;
class Paths;

class NotificationModel;

enum class Platform : uint8_t {
    Twitch,  // 0
    // Mixer,   // 1
};

class NotificationController final : public Singleton, private QObject
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isChannelNotified(const QString &channelName, Platform p);
    void updateChannelNotification(const QString &channelName, Platform p);
    void addChannelNotification(const QString &channelName, Platform p);
    void removeChannelNotification(const QString &channelName, Platform p);

    void addNotification(NotificationPopup &notif);
    void playSound();

    UnsortedSignalVector<QString> getVector(Platform p);

    std::map<Platform, UnsortedSignalVector<QString>> channelMap;

    NotificationModel *createModel(QObject *parent, Platform p);

private:
    bool initialized_ = false;

    void fetchFakeChannels();
    void removeFakeChannel(const QString channelName);
    void getFakeTwitchChannelLiveStatus(const QString &channelName);

    void startNotification();

    std::vector<QString> fakeTwitchChannels;
    QTimer *liveStatusTimer_;

    std::vector<NotificationPopup> queue;

    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
    /*
    ChatterinoSetting<std::vector<QString>> mixerSetting_ = {
        "/notifications/mixer"};
    */
};

}  // namespace chatterino
