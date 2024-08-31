#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "util/QCompareCaseInsensitive.hpp"

#include <QTimer>

namespace chatterino {

class Settings;
class Paths;
struct HelixStream;

class NotificationModel;

enum class Platform : uint8_t {
    Twitch,  // 0
};

class NotificationController final
{
public:
    NotificationController();

    // Perform an initial load so we don't have to wait for the timer
    void initialize();

    bool isChannelNotified(const QString &channelName, Platform p) const;
    void updateChannelNotification(const QString &channelName, Platform p);
    void addChannelNotification(const QString &channelName, Platform p);
    void removeChannelNotification(const QString &channelName, Platform p);

    struct NotificationPayload {
        QString channelId;
        QString channelName;
        QString displayName;
        QString title;
        bool isInitialUpdate = false;
    };

    /// @brief Sends out notifications for a channel that has gone live
    ///
    /// This doesn't check for duplicate notifications.
    void notifyTwitchChannelLive(const NotificationPayload &payload) const;

    /// @brief Sends out notifications for a channel that has gone offline
    ///
    /// This doesn't check for duplicate notifications.
    void notifyTwitchChannelOffline(const QString &id) const;

    void playSound() const;

    NotificationModel *createModel(QObject *parent, Platform p);

private:
    void fetchFakeChannels();
    void removeFakeChannel(const QString &channelName);
    void updateFakeChannel(const QString &channelName,
                           const std::optional<HelixStream> &stream);

    struct FakeChannel {
        QString id;
        bool isLive = false;
    };

    /// @brief This map tracks channels without an associated TwitchChannel
    ///
    /// These channels won't be tracked in LiveController.
    /// Channels are identified by their login name (case insensitive).
    std::map<QString, FakeChannel, QCompareCaseInsensitive> fakeChannels_;

    QTimer liveStatusTimer_;

    std::map<Platform, SignalVector<QString>> channelMap;

    ChatterinoSetting<std::vector<QString>> twitchSetting_ = {
        "/notifications/twitch"};
};

}  // namespace chatterino
