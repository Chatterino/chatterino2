#pragma once

#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "twitch-eventsub-ws/payloads/automod-message-hold-v2.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-user-message-hold-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-chat-user-message-update-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"
#include "twitch-eventsub-ws/payloads/channel-suspicious-user-message-v1.hpp"
#include "twitch-eventsub-ws/payloads/channel-suspicious-user-update-v1.hpp"

#include <QDateTime>

#include <concepts>

namespace chatterino::eventsub::detail {

template <typename T, typename... Types>
concept AnyOf = (std::same_as<T, Types> || ...);

}  // namespace chatterino::eventsub::detail

namespace chatterino::eventsub {

class EventSubMessageBuilder : public MessageBuilder
{
public:
    // builds a system message and adds a timestamp element
    EventSubMessageBuilder(TwitchChannel *channel, const QDateTime &time);
    EventSubMessageBuilder(TwitchChannel *channel);
    ~EventSubMessageBuilder();

    EventSubMessageBuilder(const EventSubMessageBuilder &) = delete;
    EventSubMessageBuilder(EventSubMessageBuilder &&) = delete;
    EventSubMessageBuilder &operator=(const EventSubMessageBuilder &) = delete;
    EventSubMessageBuilder &operator=(EventSubMessageBuilder &&) = delete;

    void appendUser(const lib::String &userName, const lib::String &userLogin,
                    QString &text, bool trailingSpace = true);

    void setMessageAndSearchText(const QString &text);

private:
    TwitchChannel *channel;
};

/// <BROADCASTER> has added <USER> as a VIP of this channel.
void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Vip &action);

/// <BROADCASTER> has removed <USER> as a VIP of this channel.
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unvip &action);

/// <MODERATOR> has warned <USER>: <REASON>
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Warn &action);

/// <MODERATOR> banned <USER>[ in <CHANNEL>]: <REASON>
void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Ban &action);

/// <MODERATOR> unbanned <USER>[ in <CHANNEL>].
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unban &action);

/// <MODERATOR> untimedout <USER>[ in <CHANNEL>].
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Untimeout &action);

/// <MODERATOR> deleted message from <USER>[ in <CHANNEL>] saying: <MESSAGE>
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Delete &action);

// mode changes

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Followers &action);
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::FollowersOff &action);

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::EmoteOnly &action);
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::EmoteOnlyOff &action);

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Slow &action);
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::SlowOff &action);

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Subscribers &action);
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::SubscribersOff &action);

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Uniquechat &action);
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::UniquechatOff &action);

/// <MODERATOR> {added/removed} <TERMS...> as (a) {blocked/permitted} term(s) on AutoMod.
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::AutomodTerms &action);

/// <BROADCASTER> modded <MODERATOR>.
void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Mod &action);

/// <BROADCASTER> unmodded <MODERATOR>.
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unmod &action);

/// <MODERATOR> initiated a raid to <CHANNEL>.
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Raid &action);

/// <MODERATOR> canceled the raid to <CHANNEL>.
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unraid &action);

MessagePtr makeAutomodHoldMessageHeader(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::automod_message_hold::v2::Event &event);

MessagePtr makeAutomodHoldMessageBody(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::automod_message_hold::v2::Event &event);

MessagePtr makeSuspiciousUserMessageHeader(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_suspicious_user_message::v1::Event &event);

MessagePtr makeSuspiciousUserMessageBody(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_suspicious_user_message::v1::Event &event);

MessagePtr makeSuspiciousUserUpdate(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_suspicious_user_update::v1::Event &event);

MessagePtr makeUserMessageHeldMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_chat_user_message_hold::v1::Event &event);

MessagePtr makeUserMessageUpdateMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_chat_user_message_update::v1::Event &event);

}  // namespace chatterino::eventsub
