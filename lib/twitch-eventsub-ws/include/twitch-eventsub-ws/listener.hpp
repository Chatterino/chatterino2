#pragma once

#include "twitch-eventsub-ws/messages/metadata.hpp"
#include "twitch-eventsub-ws/payloads/channel-ban-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-message-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"
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
        messages::Metadata metadata,
        payload::session_welcome::Payload payload) = 0;

    virtual void onNotification(messages::Metadata metadata,
                                const boost::json::value &jv) = 0;

    // Subscription types
    virtual void onChannelBan(messages::Metadata metadata,
                              payload::channel_ban::v1::Payload payload) = 0;

    virtual void onStreamOnline(
        messages::Metadata metadata,
        payload::stream_online::v1::Payload payload) = 0;

    virtual void onStreamOffline(
        messages::Metadata metadata,
        payload::stream_offline::v1::Payload payload) = 0;

    virtual void onChannelChatNotification(
        messages::Metadata metadata,
        payload::channel_chat_notification::v1::Payload payload) = 0;

    virtual void onChannelUpdate(
        messages::Metadata metadata,
        payload::channel_update::v1::Payload payload) = 0;

    virtual void onChannelChatMessage(
        messages::Metadata metadata,
        payload::channel_chat_message::v1::Payload payload) = 0;

    virtual void onChannelModerate(
        messages::Metadata metadata,
        payload::channel_moderate::v2::Payload payload) = 0;

    // Add your new subscription types above this line
};

}  // namespace chatterino::eventsub::lib
