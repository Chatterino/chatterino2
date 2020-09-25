#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/Settings.hpp"
#include "widgets/dialogs/NotificationPopup.hpp"

#include <QLayout>
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

    void addNotification(QLayout *layout, std::chrono::milliseconds time,
                         std::function<void()> callback);
    void playSound();

    SignalVector<QString> getVector(Platform p);

    std::map<Platform, SignalVector<QString>> channelMap;

    NotificationModel *createModel(QObject *parent, Platform p);

private:
    bool initialized_ = false;

    void fetchFakeChannels();
    void removeFakeChannel(const QString channelName);
    void getFakeTwitchChannelLiveStatus(const QString &channelName);

    // fakeTwitchChannels is a list of streams who are live that we have already sent out a notification for
    std::vector<QString> fakeTwitchChannels;
    QTimer *liveStatusTimer_;

    std::queue<
        std::tuple<QLayout *, std::chrono::milliseconds, std::function<void()>>>
        queue_;

    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
    /*
    ChatterinoSetting<std::vector<QString>> mixerSetting_ = {
        "/notifications/mixer"};
    */

    NotificationPopup *popupWindow_;

    void startNotification();
};

}  // namespace chatterino
