#include "providers/twitch/eventsub/Connection.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/PostToThread.hpp"

#include <boost/json.hpp>
#include <twitch-eventsub-ws/listener.hpp>
#include <twitch-eventsub-ws/session.hpp>

#include <chrono>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoTwitchEventSub;

}  // namespace

namespace chatterino::eventsub {

void Connection::onSessionWelcome(
    lib::messages::Metadata metadata,
    lib::payload::session_welcome::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On session welcome:" << payload.id.c_str();

    this->sessionID = QString::fromStdString(payload.id);
}

void Connection::onNotification(lib::messages::Metadata metadata,
                                const boost::json::value &jv)
{
    (void)metadata;
    auto jsonString = boost::json::serialize(jv);
    qCDebug(LOG) << "on notification: " << jsonString.c_str();
}

void Connection::onChannelBan(lib::messages::Metadata metadata,
                              lib::payload::channel_ban::v1::Payload payload)
{
    (void)metadata;

    auto roomID = QString::fromStdString(payload.event.broadcasterUserID);

    BanAction action{};

    action.timestamp = std::chrono::steady_clock::now();
    action.roomID = roomID;
    action.source = ActionUser{
        .id = QString::fromStdString(payload.event.moderatorUserID),
        .login = QString::fromStdString(payload.event.moderatorUserLogin),
        .displayName = QString::fromStdString(payload.event.moderatorUserName),
    };
    action.target = ActionUser{
        .id = QString::fromStdString(payload.event.userID),
        .login = QString::fromStdString(payload.event.userLogin),
        .displayName = QString::fromStdString(payload.event.userName),
    };
    action.reason = QString::fromStdString(payload.event.reason);
    if (payload.event.isPermanent)
    {
        action.duration = 0;
    }
    else
    {
        auto timeoutDuration = payload.event.timeoutDuration();
        auto timeoutDurationInSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(timeoutDuration)
                .count();
        action.duration = timeoutDurationInSeconds;
    }

    auto chan = getApp()->getTwitch()->getChannelOrEmptyByID(roomID);

    runInGuiThread([action{std::move(action)}, chan{std::move(chan)}] {
        auto time = QDateTime::currentDateTime();
        MessageBuilder msg(action, time);
        msg->flags.set(MessageFlag::PubSub);
        chan->addOrReplaceTimeout(msg.release(), QDateTime::currentDateTime());
    });
}

void Connection::onStreamOnline(
    lib::messages::Metadata metadata,
    lib::payload::stream_online::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream online event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onStreamOffline(
    lib::messages::Metadata metadata,
    lib::payload::stream_offline::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream offline event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelChatNotification(
    lib::messages::Metadata metadata,
    lib::payload::channel_chat_notification::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel chat notification for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelUpdate(
    lib::messages::Metadata metadata,
    lib::payload::channel_update::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel update for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelChatMessage(
    lib::messages::Metadata metadata,
    lib::payload::channel_chat_message::v1::Payload payload)
{
    (void)metadata;

    qCDebug(LOG) << "Channel chat message event for"
                 << payload.event.broadcasterUserLogin.c_str();
}

}  // namespace chatterino::eventsub
