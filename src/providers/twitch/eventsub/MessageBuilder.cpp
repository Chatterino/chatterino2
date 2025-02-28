#include "providers/twitch/eventsub/MessageBuilder.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "util/Helpers.hpp"

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

QString stringifyAutomodReason(const lib::automod::AutomodReason &reason,
                               QStringView /* message */)
{
    return reason.category.qt() % u" level " % QString::number(reason.level);
}

QString stringifyAutomodReason(const lib::automod::BlockedTermReason &reason,
                               QStringView message)
{
    if (reason.termsFound.empty())
    {
        return u"blocked term usage"_s;
    }

    QString msg = [&] {
        if (reason.termsFound.size() == 1)
        {
            return u"matches 1 blocked term"_s;
        }
        return u"matches %1 blocked terms"_s.arg(reason.termsFound.size());
    }();

    if (getSettings()->streamerModeHideBlockedTermText &&
        getApp()->getStreamerMode()->isEnabled())
    {
        return msg;
    }

    for (size_t i = 0; i < reason.termsFound.size(); i++)
    {
        if (i == 0)
        {
            msg.append(u" \"");
        }
        else
        {
            msg.append(u"\", \"");
        }
        msg.append(codepointSlice(message,
                                  reason.termsFound[i].boundary.startPos,
                                  reason.termsFound[i].boundary.endPos + 1));
    }
    msg.append(u'"');

    return msg;
}

// XXX: this is a duplicate from messages/MessageBuilder.cpp
EmotePtr makeAutoModBadge()
{
    return std::make_shared<Emote>(Emote{
        .name = EmoteName{},
        .images =
            ImageSet{Image::fromResourcePixmap(getResources().twitch.automod)},
        .tooltip = Tooltip{"AutoMod"},
        .homePage =
            Url{"https://dashboard.twitch.tv/settings/moderation/automod"},
    });
}

QString localizedDisplayName(
    const lib::payload::automod_message_hold::v2::Event &event)
{
    QString displayName = event.userName.qt();
    bool hasLocalizedName =
        displayName.compare(event.userLogin.qt(), Qt::CaseInsensitive) != 0;

    switch (getSettings()->usernameDisplayMode.getValue())
    {
        case UsernameDisplayMode::Username: {
            if (hasLocalizedName)
            {
                displayName = event.userLogin.qt();
            }
            break;
        }
        case UsernameDisplayMode::LocalizedName: {
            break;
        }
        case UsernameDisplayMode::UsernameAndLocalizedName: {
            if (hasLocalizedName)
            {
                displayName =
                    event.userLogin.qt() % '(' % event.userName.qt() % ')';
            }
            break;
        }
        default:
            break;
    }
    return displayName;
}

}  // namespace

namespace chatterino::eventsub {

EventSubMessageBuilder::EventSubMessageBuilder(TwitchChannel *channel,
                                               const QDateTime &time)
    : channel(channel)
{
    this->emplace<TimestampElement>(time.time());
    this->message().flags.set(MessageFlag::System, MessageFlag::EventSub);
    this->message().flags.set(MessageFlag::Timeout);  // do we need this?
    this->message().serverReceivedTime = time;
}

EventSubMessageBuilder::EventSubMessageBuilder(TwitchChannel *channel)
    : channel(channel)
{
    this->message().flags.set(MessageFlag::EventSub);
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

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::AutomodTerms &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    if (action.action == "add")
    {
        builder.emplaceSystemTextAndUpdate(u"added"_s, text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate(u"removed"_s, text);
    }

    QString terms;
    for (size_t i = 0; i < action.terms.size(); i++)
    {
        if (i != 0)
        {
            if (i == action.terms.size() - 1)
            {
                if (action.terms.size() == 2)
                {
                    terms.append(u" and ");
                }
                else
                {
                    terms.append(u", and ");
                }
            }
            else
            {
                terms.append(u", ");
            }
        }
        terms.append(u'"');
        terms.append(action.terms[i].qt());
        terms.append(u'"');
    }
    builder.emplaceSystemTextAndUpdate(terms, text);
    builder.emplaceSystemTextAndUpdate(u"as"_s, text);
    if (action.terms.size() == 1)
    {
        builder.emplaceSystemTextAndUpdate(u"a"_s, text);
    }
    builder.emplaceSystemTextAndUpdate(action.list.qt(), text);
    if (action.terms.size() == 1)
    {
        builder.emplaceSystemTextAndUpdate(u"term"_s, text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate(u"terms"_s, text);
    }
    builder.emplaceSystemTextAndUpdate(u"on AutoMod."_s, text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Mod &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate(u"modded"_s, text);
    builder.appendUser(action.userName, action.userLogin, text, false);
    builder.emplaceSystemTextAndUpdate(u"."_s, text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unmod &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate(u"unmodded"_s, text);
    builder.appendUser(action.userName, action.userLogin, text, false);
    builder.emplaceSystemTextAndUpdate(u"."_s, text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(EventSubMessageBuilder &builder,
                         const lib::payload::channel_moderate::v2::Event &event,
                         const lib::payload::channel_moderate::v2::Raid &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("initiated a raid to", text);
    builder.appendUser(action.userName, action.userLogin, text, false);
    builder.emplaceSystemTextAndUpdate(".", text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

void makeModerateMessage(
    EventSubMessageBuilder &builder,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Unraid &action)
{
    QString text;

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("canceled the raid to", text);
    builder.appendUser(action.userName, action.userLogin, text, false);
    builder.emplaceSystemTextAndUpdate(".", text);

    builder.message().messageText = text;
    builder.message().searchText = text;
}

MessagePtr makeAutomodHoldMessageHeader(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::automod_message_hold::v2::Event &event)
{
    EventSubMessageBuilder builder(channel);
    builder->serverReceivedTime = time;
    builder->id = u"automod_" + event.messageID.qt();
    builder->loginName = u"automod"_s;
    builder->channelName = event.broadcasterUserLogin.qt();
    builder->flags.set(MessageFlag::PubSub, MessageFlag::Timeout,
                       MessageFlag::AutoMod,
                       MessageFlag::AutoModOffendingMessageHeader);
    builder->flags.set(
        MessageFlag::AutoModBlockedTerm,
        std::holds_alternative<lib::automod::BlockedTermReason>(event.reason));

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::Text,
                                 QColor(0, 0, 255), FontStyle::ChatMediumBold);
    // AutoMod header message
    auto reason = std::visit(
        [&](const auto &r) {
            return stringifyAutomodReason(r, event.message.text.qt());
        },
        event.reason);
    builder.emplace<TextElement>(u"Held a message for reason: " % reason %
                                     u". Allow will post it in chat. ",
                                 MessageElementFlag::Text, MessageColor::Text);
    // Allow link button
    builder
        .emplace<TextElement>("Allow", MessageElementFlag::Text,
                              MessageColor(QColor(0, 255, 0)),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModAllow, event.messageID.qt()});
    // Deny link button
    builder
        .emplace<TextElement>(" Deny", MessageElementFlag::Text,
                              MessageColor(QColor(255, 0, 0)),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModDeny, event.messageID.qt()});
    auto text = u"AutoMod: Held a message for reason: " % reason %
                u". Allow will post "
                "it in chat. Allow Deny";
    builder->messageText = text;
    builder->searchText = text;

    return builder.release();
}

MessagePtr makeAutomodHoldMessageBody(
    TwitchChannel *channel, const QDateTime &time,
    const lib::payload::automod_message_hold::v2::Event &event)
{
    EventSubMessageBuilder builder(channel);
    builder->serverReceivedTime = time;
    builder->flags.set(MessageFlag::PubSub, MessageFlag::Timeout,
                       MessageFlag::AutoMod,
                       MessageFlag::AutoModOffendingMessage);
    builder->flags.set(
        MessageFlag::AutoModBlockedTerm,
        std::holds_alternative<lib::automod::BlockedTermReason>(event.reason));

    // Builder for offender's message
    builder->channelName = event.broadcasterUserLogin.qt();
    builder
        .emplace<TextElement>(u'#' + event.broadcasterUserLogin.qt(),
                              MessageElementFlag::ChannelName,
                              MessageColor::System)
        ->setLink({Link::JumpToChannel, event.broadcasterUserLogin.qt()});
    builder.emplace<TimestampElement>(time.time());
    builder.emplace<TwitchModerationElement>();
    builder->loginName = event.userLogin.qt();

    auto displayName = localizedDisplayName(event);
    // sender username
    builder.emplace<MentionElement>(
        displayName + ':', event.userLogin.qt(), MessageColor::Text,
        channel->getUserColor(event.userLogin.qt()));
    // sender's message caught by AutoMod
    builder.emplace<TextElement>(event.message.text.qt(),
                                 MessageElementFlag::Text, MessageColor::Text);
    auto text = displayName % u": " % event.message.text.qt();
    builder->messageText = text;
    builder->searchText = text;

    return builder.release();
}

}  // namespace chatterino::eventsub
