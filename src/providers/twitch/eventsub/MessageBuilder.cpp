#include "providers/twitch/eventsub/MessageBuilder.hpp"

#include "common/Literals.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

namespace {

using namespace chatterino;
using namespace chatterino::eventsub;
using namespace chatterino::literals;

/// <MODERATOR> turned {on/off} <MODE> mode. [<DURATION>]
void makeModeMessage(EventSubMessageBuilder &builder,
                     const lib::payload::channel_moderate::v2::Event &event,
                     const QString &mode, bool on, const QString &duration = {})
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate(u"turned"_s, text);
    QString op = on ? u"on"_s : u"off"_s;
    builder.emplaceSystemTextAndUpdate(op, text);
    builder.emplaceSystemTextAndUpdate(mode, text);
    builder.emplaceSystemTextAndUpdate(u"mode."_s, text);

    if (!duration.isEmpty())
    {
        builder.emplaceSystemTextAndUpdate(duration, text);
    }

    builder.message().messageText = text;
    builder.message().searchText = text;
}

}  // namespace

namespace chatterino::eventsub {

EventSubMessageBuilder::EventSubMessageBuilder(TwitchChannel *channel,
                                               const QDateTime &time)
    : channel(channel)
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);  // do we need this?
    this->message().serverReceivedTime = time;
}

EventSubMessageBuilder::~EventSubMessageBuilder() = default;

void EventSubMessageBuilder::appendUser(const lib::String &userName,
                                        const lib::String &userLogin,
                                        QString &text, bool trailingSpace)
{
    auto login = userLogin.qt();
    auto *el = this->emplace<MentionElement>(userName.qt(), login,
                                             MessageColor::System,
                                             channel->getUserColor(login));
    text.append(login);

    if (trailingSpace)
    {
        text.append(u' ');
    }
    else
    {
        el->setTrailingSpace(false);
    }
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Vip &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("has added", text);
    builder.appendUser(action.userName, action.userLogin, text);
    builder.emplaceSystemTextAndUpdate("as a VIP of this channel.", text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unvip &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("has removed", text);
    builder.appendUser(action.userName, action.userLogin, text);
    builder.emplaceSystemTextAndUpdate("as a VIP of this channel.", text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Warn &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("has warned", text);
    builder.appendUser(action.userName, action.userLogin, text, false);

    QStringList reasons;

    if (!action.reason.qt().isEmpty())
    {
        reasons.append(action.reason.qt());
    }

    for (const auto &rule : action.chatRulesCited)
    {
        if (!rule.qt().isEmpty())
        {
            reasons.append(rule.qt());
        }
    }

    if (reasons.isEmpty())
    {
        builder.emplaceSystemTextAndUpdate(".", text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate(":", text);
        builder.emplaceSystemTextAndUpdate(reasons.join(", "), text);
    }

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Ban &action)
{
    QString text;
    bool isShared = event.isFromSharedChat();

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("banned", text);
    builder.appendUser(action.userName, action.userLogin, text, isShared);

    if (isShared)
    {
        builder.emplaceSystemTextAndUpdate("in", text);
        builder.appendUser(*event.sourceBroadcasterUserName,
                           *event.sourceBroadcasterUserLogin, text, false);
    }

    if (action.reason.view().empty())
    {
        builder.emplaceSystemTextAndUpdate(".", text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate(":", text);
        builder.emplaceSystemTextAndUpdate(action.reason.qt(), text);
    }

    builder->messageText = text;
    builder->searchText = text;
    builder->timeoutUser = action.userLogin.qt();
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unban &action)
{
    QString text;
    bool isShared = event.isFromSharedChat();

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("unbanned", text);
    builder.appendUser(action.userName, action.userLogin, text, isShared);

    if (isShared)
    {
        builder.emplaceSystemTextAndUpdate("in", text);
        builder.appendUser(*event.sourceBroadcasterUserName,
                           *event.sourceBroadcasterUserLogin, text, false);
    }

    builder.emplaceSystemTextAndUpdate(".", text);

    builder->messageText = text;
    builder->searchText = text;
    builder->timeoutUser = action.userLogin.qt();
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Delete &action)
{
    builder.message().flags.set(MessageFlag::DoNotTriggerNotification);

    QString text;
    bool isShared = event.isFromSharedChat();

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("deleted message from", text);
    builder.appendUser(action.userName, action.userLogin, text);

    if (isShared)
    {
        builder.emplaceSystemTextAndUpdate("in", text);
        builder.appendUser(*event.sourceBroadcasterUserName,
                           *event.sourceBroadcasterUserLogin, text);
    }

    builder.emplaceSystemTextAndUpdate("saying:", text);

    if (action.messageBody.view().length() > 50)
    {
        builder
            .emplace<TextElement>(action.messageBody.qt().left(50) + "…",
                                  MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageID.qt()});

        text.append(action.messageBody.qt().left(50) + "…");
    }
    else
    {
        builder
            .emplace<TextElement>(action.messageBody.qt(),
                                  MessageElementFlag::Text, MessageColor::Text)
            ->setLink({Link::JumpToMessage, action.messageID.qt()});

        text.append(action.messageBody.qt());
    }

    builder->messageText = text;
    builder->searchText = text;
    builder->timeoutUser = action.userLogin.qt();
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Followers &action)
{
    QString duration;
    if (action.followDurationMinutes > 0)
    {
        duration = u"(%1 minutes)"_s.arg(action.followDurationMinutes);
    }
    makeModeMessage(builder, event, u"followers-only"_s, true, duration);
}
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::FollowersOff & /*action*/)
{
    makeModeMessage(builder, event, u"followers-only"_s, false);
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::EmoteOnly & /*action*/)
{
    makeModeMessage(builder, event, u"emote-only"_s, true);
}
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::EmoteOnlyOff & /*action*/)
{
    makeModeMessage(builder, event, u"emote-only"_s, false);
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Slow &action)
{
    makeModeMessage(builder, event, u"slow"_s, true,
                    u"(%1 seconds)"_s.arg(action.waitTimeSeconds));
}
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::SlowOff & /*action*/)
{
    makeModeMessage(builder, event, u"slow"_s, false);
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Subscribers & /*action*/)
{
    makeModeMessage(builder, event, u"subscribers-only"_s, true);
}
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::SubscribersOff & /*action*/)
{
    makeModeMessage(builder, event, u"subscribers-only"_s, false);
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Uniquechat & /*action*/)
{
    makeModeMessage(builder, event, u"unique-chat"_s, true);
}
void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::UniquechatOff & /*action*/)
{
    makeModeMessage(builder, event, u"unique-chat"_s, false);
}

}  // namespace chatterino::eventsub
