#include "providers/twitch/eventsub/MessageBuilder.hpp"

#include "messages/MessageBuilder.hpp"

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

    builder.emplace<MentionElement>(
        event.moderatorUserName.qt(), event.moderatorUserLogin.qt(),
        MessageColor::System,
        channel->getUserColor(event.moderatorUserLogin.qt()));
    text.append(event.moderatorUserLogin.qt() + " ");

    builder.emplaceSystemTextAndUpdate("has added", text);

    builder.emplace<MentionElement>(
        action.userName.qt(), action.userLogin.qt(), MessageColor::System,
        channel->getUserColor(action.userLogin.qt()));
    text.append(action.userLogin.qt() + " ");

    builder.emplaceSystemTextAndUpdate("as a VIP of this channel.", text);

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

    builder.emplace<MentionElement>(
        event.moderatorUserName.qt(), event.moderatorUserLogin.qt(),
        MessageColor::System,
        channel->getUserColor(event.moderatorUserLogin.qt()));
    text.append(event.moderatorUserLogin.qt() + " ");

    builder.emplaceSystemTextAndUpdate("has removed", text);

    builder.emplace<MentionElement>(
        action.userName.qt(), action.userLogin.qt(), MessageColor::System,
        channel->getUserColor(action.userLogin.qt()));
    text.append(action.userLogin.qt() + " ");

    builder.emplaceSystemTextAndUpdate("as a VIP of this channel.", text);

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

    builder.emplace<MentionElement>(
        event.moderatorUserName.qt(), event.moderatorUserLogin.qt(),
        MessageColor::System,
        channel->getUserColor(event.moderatorUserLogin.qt()));
    text.append(event.moderatorUserLogin.qt() + " ");

    builder.emplaceSystemTextAndUpdate("has warned", text);

    builder
        .emplace<MentionElement>(action.userName.qt(), action.userLogin.qt(),
                                 MessageColor::System,
                                 channel->getUserColor(action.userLogin.qt()))
        ->setTrailingSpace(false);
    text.append(action.userLogin.qt());

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
