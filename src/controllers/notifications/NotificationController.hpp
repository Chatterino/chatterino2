#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"

#include <QTimer>

namespace chatterino {

class Settings;
class Paths;

class NotificationModel;

enum class Platform : uint8_t {
    Twitch,  // 0
};

class NotificationController final : public Singleton, private QObject
{
public:
    void initialize(Settings &settings, Paths &paths) override;

    bool isChannelNotified(const QString &channelName, Platform p);
    void updateChannelNotification(const QString &channelName, Platform p);
    void addChannelNotification(const QString &channelName, Platform p);
    void removeChannelNotification(const QString &channelName, Platform p);

    void playSound();

    SignalVector<QString> getVector(Platform p);

    std::map<Platform, SignalVector<QString>> channelMap;

    NotificationModel *createModel(QObject *parent, Platform p);

private:
    bool initialized_ = false;

    void fetchFakeChannels();
    void removeFakeChannel(const QString channelName);
    void checkStream(bool live, QString channelName);

    // fakeTwitchChannels is a list of streams who are live that we have already sent out a notification for
    std::vector<QString> fakeTwitchChannels;
    QTimer *liveStatusTimer_;

    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
};

}  // namespace chatterino
