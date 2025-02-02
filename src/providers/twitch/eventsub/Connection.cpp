#include "providers/twitch/eventsub/Connection.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/eventsub/MessageBuilder.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/PostToThread.hpp"

#include <boost/json.hpp>
#include <qdatetime.h>
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

void Connection::onChannelModerate(
    lib::messages::Metadata metadata,
    lib::payload::channel_moderate::v2::Payload payload)
{
    (void)metadata;

    using lib::payload::channel_moderate::v2::Action;

    auto channelPtr = getApp()->getTwitch()->getChannelOrEmpty(
        payload.event.broadcasterUserLogin.qt());
    if (channelPtr->isEmpty())
    {
        qCDebug(LOG)
            << "Channel moderate event for broadcaster we're not interested in"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto *channel = dynamic_cast<TwitchChannel *>(channelPtr.get());
    if (channel == nullptr)
    {
        qCDebug(LOG)
            << "Channel moderate event for broadcaster is not a Twitch channel?"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    const auto now = QDateTime::currentDateTime();

    switch (payload.event.action)
    {
        case Action::Vip: {
            const auto &oAction = payload.event.vip;

            if (!oAction.has_value())
            {
                qCWarning(LOG) << "VIP action type had no VIP action body";
                return;
            }
            auto msg =
                makeVipMessage(channel, now, payload.event, oAction.value());
            runInGuiThread([channel, msg] {
                channel->addMessage(msg, MessageContext::Original);
            });
        }
        break;

        case Action::Unvip: {
            const auto &oAction = payload.event.unvip;

            if (!oAction.has_value())
            {
                qCWarning(LOG) << "UnVIP action type had no UnVIP action body";
                return;
            }
            auto msg =
                makeUnvipMessage(channel, now, payload.event, oAction.value());
            runInGuiThread([channel, msg] {
                channel->addMessage(msg, MessageContext::Original);
            });
        }
        break;

        case Action::Ban:
        case Action::Timeout:
        case Action::Unban:
        case Action::Untimeout:
        case Action::Clear:
        case Action::Emoteonly:
        case Action::Emoteonlyoff:
        case Action::Followers:
        case Action::Followersoff:
        case Action::Uniquechat:
        case Action::Uniquechatoff:
        case Action::Slow:
        case Action::Slowoff:
        case Action::Subscribers:
        case Action::Subscribersoff:
        case Action::Unraid:
        case Action::DeleteMessage:
        case Action::Raid:
        case Action::AddBlockedTerm:
        case Action::AddPermittedTerm:
        case Action::RemoveBlockedTerm:
        case Action::RemovePermittedTerm:
        case Action::Mod:
        case Action::Unmod:
        case Action::ApproveUnbanRequest:
        case Action::DenyUnbanRequest:
        case Action::Warn:
        case Action::SharedChatBan:
        case Action::SharedChatTimeout:
        case Action::SharedChatUnban:
        case Action::SharedChatUntimeout:
        case Action::SharedChatDelete:
            break;
    }
}

QString Connection::getSessionID() const
{
    return this->sessionID;
}

bool Connection::isSubscribedTo(const SubscriptionRequest &request) const
{
    return this->subscriptions.contains(request);
}

void Connection::markRequestSubscribed(const SubscriptionRequest &request)
{
    this->subscriptions.emplace(request);
}

}  // namespace chatterino::eventsub
