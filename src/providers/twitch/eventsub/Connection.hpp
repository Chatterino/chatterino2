#pragma once

#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <QString>

namespace chatterino::eventsub {

class Connection final : public lib::Listener
{
public:
    void onSessionWelcome(
        lib::messages::Metadata metadata,
        lib::payload::session_welcome::Payload payload) override;

    void onNotification(lib::messages::Metadata metadata,
                        const boost::json::value &jv) override;

    void onChannelBan(lib::messages::Metadata metadata,
                      lib::payload::channel_ban::v1::Payload payload) override;

    void onStreamOnline(
        lib::messages::Metadata metadata,
        lib::payload::stream_online::v1::Payload payload) override;

    void onStreamOffline(
        lib::messages::Metadata metadata,
        lib::payload::stream_offline::v1::Payload payload) override;

    void onChannelChatNotification(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_notification::v1::Payload payload) override;

    void onChannelUpdate(
        lib::messages::Metadata metadata,
        lib::payload::channel_update::v1::Payload payload) override;

    void onChannelChatMessage(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_message::v1::Payload payload) override;

    QString getSessionID() const
    {
        return this->sessionID;
    }

private:
    QString sessionID;
};

}  // namespace chatterino::eventsub
