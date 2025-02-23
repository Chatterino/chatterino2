#include "providers/twitch/eventsub/MessageBuilder.hpp"

#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

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

}  // namespace chatterino::eventsub
