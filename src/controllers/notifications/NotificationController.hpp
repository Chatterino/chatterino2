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

    bool isChannelNotified(const QString &channelName);

    void updateChannelNotification(const QString &channelName);
    void addChannelNotification(const QString &channelName);
    void removeChannelNotification(const QString &channelName);

    UnsortedSignalVector<QString> notificationVector;

    NotificationModel *createModel(QObject *parent);

private:
    bool initialized_ = false;

    ChatterinoSetting<std::vector<QString>> notificationSetting_ = {
        "/notifications/channels"};
};

}  // namespace chatterino
