#pragma once

#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <boost/date_time.hpp>
#include <QString>

#include <unordered_set>

namespace chatterino::eventsub {

class Connection final : public lib::Listener
{
public:
    void onSessionWelcome(
        const lib::messages::Metadata &metadata,
        const lib::payload::session_welcome::Payload &payload) override;

    void onNotification(const lib::messages::Metadata &metadata,
                        const boost::json::value &jv) override;

    void onChannelBan(
        const lib::messages::Metadata &metadata,
        const lib::payload::channel_ban::v1::Payload &payload) override;

    void onStreamOnline(
        const lib::messages::Metadata &metadata,
        const lib::payload::stream_online::v1::Payload &payload) override;

    void onStreamOffline(
        const lib::messages::Metadata &metadata,
        const lib::payload::stream_offline::v1::Payload &payload) override;

    void onChannelChatNotification(
        const lib::messages::Metadata &metadata,
        const lib::payload::channel_chat_notification::v1::Payload &payload)
        override;

    void onChannelUpdate(
        const lib::messages::Metadata &metadata,
        const lib::payload::channel_update::v1::Payload &payload) override;

    void onChannelChatMessage(
        const lib::messages::Metadata &metadata,
        const lib::payload::channel_chat_message::v1::Payload &payload)
        override;

    void onChannelModerate(
        const lib::messages::Metadata &metadata,
        const lib::payload::channel_moderate::v2::Payload &payload) override;

    QString getSessionID() const;

    bool isSubscribedTo(const SubscriptionRequest &request) const;
    void markRequestSubscribed(const SubscriptionRequest &request);
    // TODO: Add an "markRequestUnsubscribed" method

private:
    QString sessionID;

    std::unordered_set<SubscriptionRequest> subscriptions;
};

}  // namespace chatterino::eventsub
