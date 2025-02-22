#include "providers/twitch/eventsub/MessageBuilder.hpp"

#include "messages/MessageBuilder.hpp"

namespace {

using namespace chatterino;

void appendSystemUser(MessageBuilder &builder, QString &text,
                      TwitchChannel *channel, const QString &userName,
                      const QString &userLogin, bool trailingSpace = true)
{
    auto *el = builder.emplace<MentionElement>(
        userName, userLogin, MessageColor::System,
        channel->getUserColor(userLogin));
    text.append(userLogin);

    if (trailingSpace)
    {
        text.append(' ');
    }
    else
    {
        el->setTrailingSpace(false);
    }
}

void appendSystemUser(MessageBuilder &builder, QString &text,
                      TwitchChannel *channel,
                      const eventsub::lib::String &userName,
                      const eventsub::lib::String &userLogin,
                      bool trailingSpace = true)
{
    appendSystemUser(builder, text, channel, userName.qt(), userLogin.qt(),
                     trailingSpace);
}

}  // namespace

namespace chatterino::eventsub {

MessagePtr makeVipMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Vip &action)
{
    MessageBuilder builder;

    QString text;

    builder.emplace<TimestampElement>();
    builder->flags.set(MessageFlag::System);
    builder->flags.set(MessageFlag::Timeout);
    builder->loginName = event.moderatorUserLogin.qt();

    appendSystemUser(builder, text, channel, event.moderatorUserName,
                     event.moderatorUserLogin);

    builder.emplaceSystemTextAndUpdate("has added", text);
    appendSystemUser(builder, text, channel, action.userName, action.userLogin);

    builder.emplaceSystemTextAndUpdate("as a VIP of", text);

    if (event.isFromSharedChat())
    {
        appendSystemUser(builder, text, channel,
                         *event.sourceBroadcasterUserName,
                         *event.sourceBroadcasterUserLogin, false);
        builder.emplaceSystemTextAndUpdate(".", text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate("this channel.", text);
    }

    builder.message().messageText = text;
    builder.message().searchText = text;

    builder.message().serverReceivedTime = time;

    return builder.release();
}

MessagePtr makeUnvipMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unvip &action)
{
    MessageBuilder builder;

    QString text;

    builder.emplace<TimestampElement>();
    builder->flags.set(MessageFlag::System);
    builder->flags.set(MessageFlag::Timeout);
    builder->loginName = event.moderatorUserLogin.qt();

    appendSystemUser(builder, text, channel, event.moderatorUserName,
                     event.moderatorUserLogin);

    builder.emplaceSystemTextAndUpdate("has removed", text);

    appendSystemUser(builder, text, channel, action.userName, action.userLogin);

    builder.emplaceSystemTextAndUpdate("as a VIP of", text);

    if (event.isFromSharedChat())
    {
        appendSystemUser(builder, text, channel,
                         *event.sourceBroadcasterUserName,
                         *event.sourceBroadcasterUserLogin, false);
        builder.emplaceSystemTextAndUpdate(".", text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate("this channel.", text);
    }

    builder.message().messageText = text;
    builder.message().searchText = text;

    builder.message().serverReceivedTime = time;

    return builder.release();
}

MessagePtr makeWarnMessage(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Warn &action)
{
    MessageBuilder builder;

    QString text;

    builder.emplace<TimestampElement>();
    builder->flags.set(MessageFlag::System);
    builder->flags.set(MessageFlag::Timeout);
    builder->loginName = event.moderatorUserLogin.qt();

    appendSystemUser(builder, text, channel, event.moderatorUserName,
                     event.moderatorUserLogin);

    builder.emplaceSystemTextAndUpdate("has warned", text);

    bool isShared = event.isFromSharedChat();
    appendSystemUser(builder, text, channel, action.userName, action.userLogin,
                     isShared);

    if (isShared)
    {
        builder.emplaceSystemTextAndUpdate("in", text);
        appendSystemUser(builder, text, channel,
                         *event.sourceBroadcasterUserName,
                         *event.sourceBroadcasterUserLogin, false);
    }

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

    builder.message().serverReceivedTime = time;

    return builder.release();
}

}  // namespace chatterino::eventsub
