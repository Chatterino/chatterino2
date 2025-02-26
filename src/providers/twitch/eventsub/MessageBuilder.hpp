#pragma once

#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

#include <QDateTime>

namespace chatterino::eventsub {

class EventSubMessageBuilder : public MessageBuilder
{
public:
    EventSubMessageBuilder(TwitchChannel *channel, const QDateTime &time);
    ~EventSubMessageBuilder();

    EventSubMessageBuilder(const EventSubMessageBuilder &) = delete;
    EventSubMessageBuilder(EventSubMessageBuilder &&) = delete;
    EventSubMessageBuilder &operator=(const EventSubMessageBuilder &) = delete;
    EventSubMessageBuilder &operator=(EventSubMessageBuilder &&) = delete;

    void appendUser(const lib::String &userName, const lib::String &userLogin,
                    QString &text, bool trailingSpace = true);

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

/// <MODERATOR> deleted message from <USER>[ in <CHANNEL>] saying: <MESSAGE>
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Delete &action);

}  // namespace chatterino::eventsub
