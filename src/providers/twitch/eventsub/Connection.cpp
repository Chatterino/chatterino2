#include "providers/twitch/eventsub/Connection.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/eventsub/MessageBuilder.hpp"
#include "providers/twitch/eventsub/MessageHandlers.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"

#include <boost/json.hpp>
#include <qdatetime.h>
#include <twitch-eventsub-ws/listener.hpp>
#include <twitch-eventsub-ws/session.hpp>

#include <chrono>

namespace {

using namespace chatterino;
using namespace chatterino::eventsub;

namespace channel_moderate = lib::payload::channel_moderate::v2;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoTwitchEventSub;

template <typename Action>
concept CanMakeModMessage = requires(
    EventSubMessageBuilder &builder, const channel_moderate::Event &event,
    const std::remove_cvref_t<Action> &action) {
    makeModerateMessage(builder, event, action);
};

template <typename Action>
concept CanHandleModMessage =
    requires(TwitchChannel *channel, const QDateTime &time,
             const channel_moderate::Event &event,
             const std::remove_cvref_t<Action> &action) {
        handleModerateMessage(channel, time, event, action);
    };

}  // namespace

namespace chatterino::eventsub {

void Connection::onSessionWelcome(
    const lib::messages::Metadata &metadata,
    const lib::payload::session_welcome::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On session welcome:" << payload.id.c_str();

    this->sessionID = QString::fromStdString(payload.id);
}

void Connection::onNotification(const lib::messages::Metadata &metadata,
                                const boost::json::value &jv)
{
    (void)metadata;
    auto jsonString = boost::json::serialize(jv);
    qCDebug(LOG) << "on notification: " << jsonString.c_str();
}

void Connection::onChannelBan(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_ban::v1::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel ban event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onStreamOnline(
    const lib::messages::Metadata &metadata,
    const lib::payload::stream_online::v1::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream online event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onStreamOffline(
    const lib::messages::Metadata &metadata,
    const lib::payload::stream_offline::v1::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream offline event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelChatNotification(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_chat_notification::v1::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel chat notification for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelUpdate(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_update::v1::Payload &payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel update for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelChatMessage(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_chat_message::v1::Payload &payload)
{
    (void)metadata;

    qCDebug(LOG) << "Channel chat message event for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void Connection::onChannelModerate(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_moderate::v2::Payload &payload)
{
    (void)metadata;

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

    auto now = QDateTime::fromStdTimePoint(
        std::chrono::time_point_cast<std::chrono::milliseconds>(
            metadata.messageTimestamp));

    std::visit(
        [&](auto &&action) {
            using Action = std::remove_cvref_t<decltype(action)>;
            if constexpr (CanMakeModMessage<Action>)
            {
                EventSubMessageBuilder builder(channel, now);
                builder->loginName = payload.event.moderatorUserLogin.qt();
                makeModerateMessage(builder, payload.event, action);
                auto msg = builder.release();
                runInGuiThread([channel, msg] {
                    channel->addMessage(msg, MessageContext::Original);
                });
            }

            if constexpr (CanHandleModMessage<Action>)
            {
                handleModerateMessage(channel, now, payload.event, action);
            }
        },
        payload.event.action);
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
