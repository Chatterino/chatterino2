#include "providers/twitch/eventsub/Connection.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/eventsub/Controller.hpp"
#include "providers/twitch/eventsub/MessageBuilder.hpp"
#include "providers/twitch/eventsub/MessageHandlers.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/PostToThread.hpp"

#include <boost/json.hpp>
#include <QDateTime>
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

void Connection::onClose(std::unique_ptr<lib::Listener> self,
                         const std::optional<std::string> &reconnectURL)
{
    auto *app = tryGetApp();
    if (!app)
    {
        return;
    }

    app->getEventSub()->reconnectConnection(std::move(self), reconnectURL,
                                            this->subscriptions);
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

    auto now = chronoToQDateTime(metadata.messageTimestamp);

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

void Connection::onAutomodMessageHold(
    const lib::messages::Metadata &metadata,
    const lib::payload::automod_message_hold::v2::Payload &payload)
{
    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG)
            << "Automod message hold for broadcaster we're not interested in"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto time = chronoToQDateTime(metadata.messageTimestamp);
    auto header = makeAutomodHoldMessageHeader(channel, time, payload.event);
    auto body = makeAutomodHoldMessageBody(channel, time, payload.event);

    auto messageText = payload.event.message.text.qt();
    auto userLogin = payload.event.userLogin.qt();

    runInGuiThread([channel, messageText, userLogin, header, body] {
        auto [highlighted, highlightResult] = getApp()->getHighlights()->check(
            {}, {}, userLogin, messageText, body->flags);
        if (highlighted)
        {
            MessageBuilder::triggerHighlights(
                channel,
                {
                    .customSound =
                        highlightResult.customSoundUrl.value_or<QUrl>({}),
                    .playSound = highlightResult.playSound,
                    .windowAlert = highlightResult.alert,
                });
        }

        channel->addMessage(header, MessageContext::Original);
        channel->addMessage(body, MessageContext::Original);

        getApp()->getTwitch()->getAutomodChannel()->addMessage(
            header, MessageContext::Original);
        getApp()->getTwitch()->getAutomodChannel()->addMessage(
            body, MessageContext::Original);

        if (getSettings()->showAutomodInMentions)
        {
            getApp()->getTwitch()->getMentionsChannel()->addMessage(
                header, MessageContext::Original);
            getApp()->getTwitch()->getMentionsChannel()->addMessage(
                body, MessageContext::Original);
        }
    });
}
void Connection::onAutomodMessageUpdate(
    const lib::messages::Metadata & /*metadata*/,
    const lib::payload::automod_message_update::v2::Payload &payload)
{
    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG)
            << "Automod message hold for broadcaster we're not interested in"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    // Gray out approve/deny button upon "ALLOWED" and "DENIED" statuses
    // They are versions of automod_message_(denied|approved) but for mods.
    auto id = "automod_" + payload.event.messageID.qt();
    runInGuiThread([channel, id] {
        channel->disableMessage(id);
    });
}

void Connection::onChannelSuspiciousUserMessage(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_suspicious_user_message::v1::Payload &payload)
{
    // monitored chats are received over irc; in the future, we will use eventsub instead
    if (payload.event.lowTrustStatus !=
        lib::suspicious_users::Status::Restricted)
    {
        return;
    }

    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG)
            << "Suspicious message for broadcaster we're not interested in"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto time = chronoToQDateTime(metadata.messageTimestamp);
    auto header = makeSuspiciousUserMessageHeader(channel, time, payload.event);
    auto body = makeSuspiciousUserMessageBody(channel, time, payload.event);

    runInGuiThread([channel, header, body] {
        channel->addMessage(header, MessageContext::Original);
        channel->addMessage(body, MessageContext::Original);
    });
}

void Connection::onChannelSuspiciousUserUpdate(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_suspicious_user_update::v1::Payload &payload)
{
    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG) << "Channel Suspicious User Update for broadcaster we're "
                        "not interested in"
                     << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto time = chronoToQDateTime(metadata.messageTimestamp);
    auto message = makeSuspiciousUserUpdate(channel, time, payload.event);

    runInGuiThread([channel, message] {
        channel->addMessage(message, MessageContext::Original);
    });
}

void Connection::onChannelChatUserMessageHold(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_chat_user_message_hold::v1::Payload &payload)
{
    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG) << "Channel Chat User Message Hold for broadcaster we're "
                        "not interested in"
                     << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto time = chronoToQDateTime(metadata.messageTimestamp);
    auto message = makeUserMessageHeldMessage(channel, time, payload.event);

    runInGuiThread([channel, message] {
        channel->addMessage(message, MessageContext::Original);
    });
}

void Connection::onChannelChatUserMessageUpdate(
    const lib::messages::Metadata &metadata,
    const lib::payload::channel_chat_user_message_update::v1::Payload &payload)
{
    auto *channel = dynamic_cast<TwitchChannel *>(
        getApp()
            ->getTwitch()
            ->getChannelOrEmpty(payload.event.broadcasterUserLogin.qt())
            .get());
    if (!channel || channel->isEmpty())
    {
        qCDebug(LOG)
            << "Channel Chat User Message Update for broadcaster we're "
               "not interested in"
            << payload.event.broadcasterUserLogin.qt();
        return;
    }

    auto time = chronoToQDateTime(metadata.messageTimestamp);
    auto message = makeUserMessageUpdateMessage(channel, time, payload.event);

    runInGuiThread([channel, message] {
        channel->addMessage(message, MessageContext::Original);
    });
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

void Connection::markRequestUnsubscribed(const SubscriptionRequest &request)
{
    this->subscriptions.erase(request);
}

}  // namespace chatterino::eventsub
