#pragma once

#include "common/SignalVector.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class Settings;
class Paths;

class NotificationModel;

class NotificationController final : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isChannelNotified(const QString &channelName, int &i);

    void updateChannelNotification(const QString &channelName, int &i);
    void addChannelNotification(const QString &channelName,
                                UnsortedSignalVector<QString> &vector);
    void removeChannelNotification(const QString &channelName,
                                   UnsortedSignalVector<QString> &vector);

    UnsortedSignalVector<QString> getVector(int &i);

    UnsortedSignalVector<QString> twitchVector;
    UnsortedSignalVector<QString> mixerVector;

    NotificationModel *createModel(QObject *parent, int &i);

private:
    bool initialized_ = false;
    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
    ChatterinoSetting<std::vector<QString>> mixerSetting_ = {
        "/notifications/mixer"};
};

}  // namespace chatterino
