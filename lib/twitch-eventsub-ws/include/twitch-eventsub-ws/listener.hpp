#pragma once

#include "twitch-eventsub-ws/messages/metadata.hpp"
#include "twitch-eventsub-ws/payloads/automod-message-hold-v2.hpp"
#include "twitch-eventsub-ws/payloads/automod-message-update-v2.hpp"
#include "twitch-eventsub-ws/payloads/channel-ban-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-message-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-user-message-hold-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-user-message-update-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"
#include "twitch-eventsub-ws/payloads/channel-suspicious-user-message-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-suspicious-user-update-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-update-v1.hpp"
#include "twitch-eventsub-ws/payloads/session-welcome.hpp"
#include "twitch-eventsub-ws/payloads/stream-offline-v1.hpp"
#include "twitch-eventsub-ws/payloads/stream-online-v1.hpp"

namespace chatterino::eventsub::lib {

class Listener
{
public:
    virtual ~Listener() = default;

    virtual void onSessionWelcome(
        const messages::Metadata &metadata,
        const payload::session_welcome::Payload &payload) = 0;

    virtual void onNotification(const messages::Metadata &metadata,
                                const boost::json::value &jv) = 0;

    virtual void onClose(
        std::unique_ptr<Listener> self,
        const std::optional<std::string> & /* reconnectUrl */) {};

    // Subscription types
    virtual void onChannelBan(
        const messages::Metadata &metadata,
        const payload::channel_ban::v1::Payload &payload) = 0;

    virtual void onStreamOnline(
        const messages::Metadata &metadata,
        const payload::stream_online::v1::Payload &payload) = 0;

    virtual void onStreamOffline(
        const messages::Metadata &metadata,
        const payload::stream_offline::v1::Payload &payload) = 0;

    virtual void onChannelChatNotification(
        const messages::Metadata &metadata,
        const payload::channel_chat_notification::v1::Payload &payload) = 0;

    virtual void onChannelUpdate(
        const messages::Metadata &metadata,
        const payload::channel_update::v1::Payload &payload) = 0;

    virtual void onChannelChatMessage(
        const messages::Metadata &metadata,
        const payload::channel_chat_message::v1::Payload &payload) = 0;

    virtual void onChannelModerate(
        const messages::Metadata &metadata,
        const payload::channel_moderate::v2::Payload &payload) = 0;

    virtual void onAutomodMessageHold(
        const messages::Metadata &metadata,
        const payload::automod_message_hold::v2::Payload &payload) = 0;

    virtual void onAutomodMessageUpdate(
        const messages::Metadata &metadata,
        const payload::automod_message_update::v2::Payload &payload) = 0;

    virtual void onChannelSuspiciousUserMessage(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_message::v1::Payload
            &payload) = 0;

    virtual void onChannelSuspiciousUserUpdate(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_update::v1::Payload
            &payload) = 0;

    virtual void onChannelChatUserMessageHold(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_hold::v1::Payload
            &payload) = 0;

    virtual void onChannelChatUserMessageUpdate(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_update::v1::Payload
            &payload) = 0;
    // Add your new subscription types above this line
};

}  // namespace chatterino::eventsub::lib
