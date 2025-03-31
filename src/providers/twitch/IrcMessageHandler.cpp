#include "providers/twitch/IrcMessageHandler.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "messages/Link.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"
#include "messages/MessageSink.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"

#include <IrcMessage>
#include <QLocale>
#include <QStringBuilder>

#include <memory>

using namespace chatterino::literals;

namespace {

using namespace chatterino;

// Message types below are the ones that might contain special user's message on USERNOTICE
const QSet<QString> SPECIAL_MESSAGE_TYPES{
    "sub",              //
    "subgift",          //
    "resub",            // resub messages
    "bitsbadgetier",    // bits badge upgrade
    "ritual",           // new viewer ritual
    "announcement",     // new mod announcement thing
    "viewermilestone",  // watch streak, but other categories possible in future
};

const QString ANONYMOUS_GIFTER_ID = "274598607";

MessagePtr generateBannedMessage(bool confirmedBan)
{
    const auto linkColor = MessageColor(MessageColor::Link);
    const auto accountsLink = Link(Link::Reconnect, QString());
    const auto bannedText =
        confirmedBan
            ? QString("You were banned from this channel!")
            : QString(
                  "Your connection to this channel was unexpectedly dropped.");

    const auto reconnectPromptText =
        confirmedBan
            ? QString(
                  "If you believe you have been unbanned, try reconnecting.")
            : QString("Try reconnecting.");

    MessageBuilder builder;
    auto text = QString("%1 %2").arg(bannedText, reconnectPromptText);
    builder.message().messageText = text;
    builder.message().searchText = text;
    builder.message().flags.set(MessageFlag::System);

    builder.emplace<TimestampElement>();
    builder.emplace<TextElement>(bannedText, MessageElementFlag::Text,
                                 MessageColor::System);
    builder
        .emplace<TextElement>(reconnectPromptText, MessageElementFlag::Text,
                              linkColor)
        ->setLink(accountsLink);

    return builder.release();
}

int stripLeadingReplyMention(const QVariantMap &tags, QString &content)
{
    if (!getSettings()->stripReplyMention)
    {
        return 0;
    }
    if (getSettings()->hideReplyContext)
    {
        // Never strip reply mentions if reply contexts are hidden
        return 0;
    }

    if (const auto it = tags.find("reply-parent-display-name");
        it != tags.end())
    {
        auto displayName = it.value().toString();

        if (content.length() <= 1 + displayName.length())
        {
            // The reply contains no content
            return 0;
        }

        if (content.startsWith('@') &&
            content.at(1 + displayName.length()) == ' ' &&
            content.indexOf(displayName, 1) == 1)
        {
            int messageOffset = 1 + displayName.length() + 1;
            content.remove(0, messageOffset);
            return messageOffset;
        }
    }
    return 0;
}

void checkThreadSubscription(const QVariantMap &tags,
                             const QString &senderLogin,
                             std::shared_ptr<MessageThread> &thread)
{
    if (thread->subscribed() || thread->unsubscribed())
    {
        return;
    }

    if (getSettings()->autoSubToParticipatedThreads)
    {
        const auto &currentLogin =
            getApp()->getAccounts()->twitch.getCurrent()->getUserName();

        if (senderLogin == currentLogin)
        {
            thread->markSubscribed();
        }
        else if (const auto it = tags.find("reply-parent-user-login");
                 it != tags.end())
        {
            auto name = it.value().toString();
            if (name == currentLogin)
            {
                thread->markSubscribed();
            }
        }
    }
}

ChannelPtr channelOrEmptyByTarget(const QString &target,
                                  ITwitchIrcServer &server)
{
    QString channelName;
    if (!trimChannelName(target, channelName))
    {
        return Channel::getEmpty();
    }

    return server.getChannelOrEmpty(channelName);
}

QMap<QString, QString> parseBadges(const QString &badgesString)
{
    QMap<QString, QString> badges;

    for (const auto &badgeData : badgesString.split(','))
    {
        auto parts = badgeData.split('/');
        if (parts.length() != 2)
        {
            continue;
        }

        badges.insert(parts[0], parts[1]);
    }

    return badges;
}

struct ReplyContext {
    std::shared_ptr<MessageThread> thread;
    MessagePtr parent;
};

std::optional<ClearChatMessage> parseClearChatMessage(
    Communi::IrcMessage *message)
{
    // check parameter count
    if (message->parameters().length() < 1)
    {
        return std::nullopt;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1)
    {
        return ClearChatMessage{
            .message = MessageBuilder::makeClearChatMessage(
                calculateMessageTime(message), {}),
            .disableAllMessages = true,
        };
    }

    // get username, duration and message of the timed out user
    QString username = message->parameter(1);
    QString durationInSeconds;
    QVariant v = message->tag("ban-duration");
    if (v.isValid())
    {
        durationInSeconds = v.toString();
    }

    auto timeoutMsg =
        MessageBuilder(timeoutMessage, username, durationInSeconds, false,
                       calculateMessageTime(message))
            .release();

    return ClearChatMessage{.message = timeoutMsg, .disableAllMessages = false};
}

/**
 * Parse a single IRC NOTICE message into a Chatterino message
 **/
MessagePtr parseNoticeMessage(Communi::IrcNoticeMessage *message)
{
    assert(message != nullptr);

    if (message->content().startsWith("Login auth", Qt::CaseInsensitive))
    {
        const auto linkColor = MessageColor(MessageColor::Link);
        const auto accountsLink = Link(Link::OpenAccountsPage, QString());
        const auto curUser = getApp()->getAccounts()->twitch.getCurrent();
        const auto expirationText = QString("Login expired for user \"%1\"!")
                                        .arg(curUser->getUserName());
        const auto loginPromptText = QString("Try adding your account again.");

        MessageBuilder builder;
        auto text = QString("%1 %2").arg(expirationText, loginPromptText);
        builder.message().messageText = text;
        builder.message().searchText = text;
        builder.message().flags.set(MessageFlag::System);
        builder.message().flags.set(MessageFlag::DoNotTriggerNotification);

        builder.emplace<TimestampElement>();
        builder.emplace<TextElement>(expirationText, MessageElementFlag::Text,
                                     MessageColor::System);
        builder
            .emplace<TextElement>(loginPromptText, MessageElementFlag::Text,
                                  linkColor)
            ->setLink(accountsLink);

        return builder.release();
    }

    if (message->content().startsWith("You are permanently banned "))
    {
        return {generateBannedMessage(true)};
    }

    if (message->tags().value("msg-id") == "msg_timedout")
    {
        QString remainingTime =
            formatTime(message->content().split(" ").value(5));
        QString formattedMessage =
            QString("You are timed out for %1.")
                .arg(remainingTime.isEmpty() ? "0s" : remainingTime);

        return makeSystemMessage(formattedMessage,
                                 calculateMessageTime(message).time());
    }

    // default case
    return makeSystemMessage(message->content(),
                             calculateMessageTime(message).time());
}

}  // namespace

namespace chatterino {

using namespace literals;

IrcMessageHandler &IrcMessageHandler::instance()
{
    static IrcMessageHandler instance;
    return instance;
}

void IrcMessageHandler::parseMessageInto(Communi::IrcMessage *message,
                                         MessageSink &sink,
                                         TwitchChannel *channel)
{
    auto command = message->command();

    if (command == u"PRIVMSG"_s)
    {
        parsePrivMessageInto(
            dynamic_cast<Communi::IrcPrivateMessage *>(message), sink, channel);
    }
    else if (command == u"USERNOTICE"_s)
    {
        parseUserNoticeMessageInto(message, sink, channel);
    }

    if (command == u"NOTICE"_s)
    {
        sink.addMessage(parseNoticeMessage(
                            dynamic_cast<Communi::IrcNoticeMessage *>(message)),
                        MessageContext::Original);
    }

    if (command == u"CLEARCHAT"_s)
    {
        auto cc = parseClearChatMessage(message);
        if (!cc)
        {
            return;
        }
        auto &clearChat = *cc;
        auto time = calculateMessageTime(message);
        if (clearChat.disableAllMessages)
        {
            sink.addOrReplaceClearChat(std::move(clearChat.message), time);
        }
        else
        {
            sink.addOrReplaceTimeout(std::move(clearChat.message), time);
        }
    }
}

void IrcMessageHandler::handlePrivMessage(Communi::IrcPrivateMessage *message,
                                          ITwitchIrcServer &twitchServer)
{
    auto chan = channelOrEmptyByTarget(message->target(), twitchServer);
    if (chan->isEmpty())
    {
        return;
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(chan.get());
    if (!twitchChannel)
    {
        return;
    }

    parsePrivMessageInto(message, *twitchChannel, twitchChannel);
}

void IrcMessageHandler::parsePrivMessageInto(
    Communi::IrcPrivateMessage *message, MessageSink &sink,
    TwitchChannel *channel)
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    if (message->tag("user-id") == currentUser->getUserId())
    {
        auto badgesTag = message->tag("badges");
        if (badgesTag.isValid())
        {
            auto parsedBadges = parseBadges(badgesTag.toString());
            channel->setMod(parsedBadges.contains("moderator"));
            channel->setVIP(parsedBadges.contains("vip"));
            channel->setStaff(parsedBadges.contains("staff"));
        }
    }

    IrcMessageHandler::addMessage(
        message, sink, channel, unescapeZeroWidthJoiner(message->content()),
        *getApp()->getTwitch(), false, message->isAction());

    if (message->tags().contains(u"pinned-chat-paid-amount"_s))
    {
        auto ptr = MessageBuilder::buildHypeChatMessage(message);
        if (ptr)
        {
            sink.addMessage(ptr, MessageContext::Original);
        }
    }
}

void IrcMessageHandler::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();

    // get Twitch channel
    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName))
    {
        return;
    }
    auto chan = getApp()->getTwitch()->getChannelOrEmpty(chanName);

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(chan.get());
    if (!twitchChannel)
    {
        return;
    }

    // room-id

    if (auto it = tags.find("room-id"); it != tags.end())
    {
        auto roomId = it.value().toString();
        twitchChannel->setRoomId(roomId);
    }

    // Room modes
    {
        auto roomModes = *twitchChannel->accessRoomModes();

        if (auto it = tags.find("emote-only"); it != tags.end())
        {
            roomModes.emoteOnly = it.value() == "1";
        }
        if (auto it = tags.find("subs-only"); it != tags.end())
        {
            roomModes.submode = it.value() == "1";
        }
        if (auto it = tags.find("slow"); it != tags.end())
        {
            roomModes.slowMode = it.value().toInt();
        }
        if (auto it = tags.find("r9k"); it != tags.end())
        {
            roomModes.r9k = it.value() == "1";
        }
        if (auto it = tags.find("followers-only"); it != tags.end())
        {
            roomModes.followerOnly = it.value().toInt();
        }
        twitchChannel->setRoomModes(roomModes);
    }

    twitchChannel->roomModesChanged.invoke();
}

void IrcMessageHandler::handleClearChatMessage(Communi::IrcMessage *message)
{
    auto cc = parseClearChatMessage(message);
    if (!cc)
    {
        return;
    }
    auto &clearChat = *cc;

    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName))
    {
        return;
    }

    // get channel
    auto chan = getApp()->getTwitch()->getChannelOrEmpty(chanName);

    if (chan->isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "[IrcMessageHandler::handleClearChatMessage] Twitch channel"
            << chanName << "not found";
        return;
    }

    auto time = calculateMessageTime(message);
    // chat has been cleared by a moderator
    if (clearChat.disableAllMessages)
    {
        chan->disableAllMessages();
        chan->addOrReplaceClearChat(std::move(clearChat.message), time);
    }
    else
    {
        chan->addOrReplaceTimeout(std::move(clearChat.message), time);
    }

    if (getSettings()->hideModerated)
    {
        // XXX: This is expensive. We could use a layout request if the layout
        //      would store the previous message flags.
        getApp()->getWindows()->forceLayoutChannelViews();
    }
}

void IrcMessageHandler::handleClearMessageMessage(Communi::IrcMessage *message)
{
    // check parameter count
    if (message->parameters().length() < 1)
    {
        return;
    }

    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName))
    {
        return;
    }

    // get channel
    auto chan = getApp()->getTwitch()->getChannelOrEmpty(chanName);

    if (chan->isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "[IrcMessageHandler:handleClearMessageMessage] Twitch "
               "channel"
            << chanName << "not found";
        return;
    }

    auto tags = message->tags();

    QString targetID = tags.value("target-msg-id").toString();

    auto msg = chan->findMessageByID(targetID);
    if (msg == nullptr)
    {
        return;
    }

    msg->flags.set(MessageFlag::Disabled);
    if (!getSettings()->hideDeletionActions)
    {
        chan->addMessage(MessageBuilder::makeDeletionMessageFromIRC(msg),
                         MessageContext::Original);
    }

    if (getSettings()->hideModerated && !tags.contains("historical"))
    {
        // XXX: This is expensive. We could use a layout request if the layout
        //      would store the previous message flags.
        getApp()->getWindows()->forceLayoutChannelViews();
    }
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    QString channelName;
    if (!trimChannelName(message->parameter(0), channelName))
    {
        return;
    }

    auto c = getApp()->getTwitch()->getChannelOrEmpty(channelName);
    if (c->isEmpty())
    {
        return;
    }

    // Checking if currentUser is a VIP or staff member
    QVariant badgesTag = message->tag("badges");
    if (badgesTag.isValid())
    {
        auto *tc = dynamic_cast<TwitchChannel *>(c.get());
        if (tc != nullptr)
        {
            auto parsedBadges = parseBadges(badgesTag.toString());
            tc->setVIP(parsedBadges.contains("vip"));
            tc->setStaff(parsedBadges.contains("staff"));
        }
    }

    // Checking if currentUser is a moderator
    QVariant modTag = message->tag("mod");
    if (modTag.isValid())
    {
        auto *tc = dynamic_cast<TwitchChannel *>(c.get());
        if (tc != nullptr)
        {
            tc->setMod(modTag == "1");
        }
    }
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *ircMessage)
{
    MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto *c = getApp()->getTwitch()->getWhispersChannel().get();

    auto [message, alert] = MessageBuilder::makeIrcMessage(
        c, ircMessage, args, unescapeZeroWidthJoiner(ircMessage->parameter(1)),
        0);
    if (!message)
    {
        return;
    }

    message->flags.set(MessageFlag::Whisper);
    MessageBuilder::triggerHighlights(c, alert);

    getApp()->getTwitch()->setLastUserThatWhisperedMe(message->loginName);

    if (message->flags.has(MessageFlag::ShowInMentions))
    {
        getApp()->getTwitch()->getMentionsChannel()->addMessage(
            message, MessageContext::Original);
    }

    c->addMessage(message, MessageContext::Original);

    auto overrideFlags = std::optional<MessageFlags>(message->flags);
    overrideFlags->set(MessageFlag::DoNotTriggerNotification);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers &&
        !(getSettings()->streamerModeSuppressInlineWhispers &&
          getApp()->getStreamerMode()->isEnabled()))
    {
        getApp()->getTwitch()->forEachChannel([&message, overrideFlags](
                                                  ChannelPtr channel) {
            channel->addMessage(message, MessageContext::Repost, overrideFlags);
        });
    }
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message,
                                                ITwitchIrcServer &twitchServer)
{
    auto target = message->parameter(0);
    auto *channel = dynamic_cast<TwitchChannel *>(
        twitchServer.getChannelOrEmpty(target).get());
    if (!channel)
    {
        return;
    }
    parseUserNoticeMessageInto(message, *channel, channel);
}

void IrcMessageHandler::parseUserNoticeMessageInto(Communi::IrcMessage *message,
                                                   MessageSink &sink,
                                                   TwitchChannel *channel)
{
    auto tags = message->tags();
    auto parameters = message->parameters();

    QString msgType = tags.value("msg-id").toString();
    bool mirrored = msgType == "sharedchatnotice";
    if (mirrored)
    {
        msgType = tags.value("source-msg-id").toString();
    }
    else
    {
        auto rIt = tags.find("room-id");
        auto sIt = tags.find("source-room-id");
        if (rIt != tags.end() && sIt != tags.end())
        {
            mirrored = rIt.value().toString() != sIt.value().toString();
        }
    }

    if (mirrored && msgType != "announcement")
    {
        // avoid confusing broadcasters with user payments to other channels
        return;
    }

    QString content;
    if (parameters.size() >= 2)
    {
        content = parameters[1];
    }

    if (isIgnoredMessage({
            .message = content,
            .twitchUserID = tags.value("user-id").toString(),
            .isMod = channel->isMod(),
            .isBroadcaster = channel->isBroadcaster(),
        }))
    {
        return;
    }

    if (SPECIAL_MESSAGE_TYPES.contains(msgType))
    {
        // Messages are not required, so they might be empty
        if (!content.isEmpty())
        {
            addMessage(message, sink, channel, content, *getApp()->getTwitch(),
                       true, false);
        }
    }

    auto it = tags.find("system-msg");

    if (it != tags.end())
    {
        // By default, we return value of system-msg tag
        QString messageText = it.value().toString();

        if (msgType == "bitsbadgetier")
        {
            messageText =
                QString("%1 just earned a new %2 Bits badge!")
                    .arg(tags.value("display-name").toString(),
                         kFormatNumbers(
                             tags.value("msg-param-threshold").toInt()));
        }
        else if (msgType == "announcement")
        {
            messageText = "Announcement";
        }
        else if (msgType == "subgift")
        {
            if (auto monthsIt = tags.find("msg-param-gift-months");
                monthsIt != tags.end())
            {
                int months = monthsIt.value().toInt();
                if (months > 1)
                {
                    auto plan = tags.value("msg-param-sub-plan").toString();
                    QString name =
                        ANONYMOUS_GIFTER_ID == tags.value("user-id").toString()
                            ? "An anonymous user"
                            : tags.value("display-name").toString();
                    messageText =
                        QString("%1 gifted %2 months of a Tier %3 sub to %4!")
                            .arg(name, QString::number(months),
                                 plan.isEmpty() ? '1' : plan.at(0),
                                 tags.value("msg-param-recipient-display-name")
                                     .toString());

                    if (auto countIt = tags.find("msg-param-sender-count");
                        countIt != tags.end())
                    {
                        int count = countIt.value().toInt();
                        if (count > months)
                        {
                            messageText +=
                                QString(
                                    " They've gifted %1 months in the channel.")
                                    .arg(QString::number(count));
                        }
                    }
                }
            }

            // subgifts are special because they include two users
            auto msg = MessageBuilder::makeSubgiftMessage(
                parseTagString(messageText), tags,
                calculateMessageTime(message).time());

            msg->flags.set(MessageFlag::Subscription);
            if (mirrored)
            {
                msg->flags.set(MessageFlag::SharedMessage);
            }

            sink.addMessage(msg, MessageContext::Original);
            return;
        }
        else if (msgType == "sub" || msgType == "resub")
        {
            if (auto tenure = tags.find("msg-param-multimonth-tenure");
                tenure != tags.end() && tenure.value().toInt() == 0)
            {
                int months =
                    tags.value("msg-param-multimonth-duration").toInt();
                if (months > 1)
                {
                    int tier = tags.value("msg-param-sub-plan").toInt() / 1000;
                    messageText =
                        QString(
                            "%1 subscribed at Tier %2 for %3 months in advance")
                            .arg(tags.value("display-name").toString(),
                                 QString::number(tier),
                                 QString::number(months));
                    if (msgType == "resub")
                    {
                        int cumulative =
                            tags.value("msg-param-cumulative-months").toInt();
                        messageText +=
                            QString(", reaching %1 months cumulatively so far!")
                                .arg(QString::number(cumulative));
                    }
                    else
                    {
                        messageText += "!";
                    }
                }
            }
        }

        auto displayName = [&] {
            if (msgType == u"raid")
            {
                return tags.value("msg-param-displayName").toString();
            }
            return tags.value("display-name").toString();
        }();
        auto login = tags.value("login").toString();
        if (displayName.isEmpty())
        {
            displayName = login;
        }

        MessageColor userColor = MessageColor::System;
        if (auto colorTag = tags.value("color").value<QColor>();
            colorTag.isValid())
        {
            userColor = MessageColor(colorTag);
        }

        auto msg = MessageBuilder::makeSystemMessageWithUser(
            parseTagString(messageText), login, displayName, userColor,
            calculateMessageTime(message).time());

        msg->flags.set(MessageFlag::Subscription);
        if (mirrored)
        {
            msg->flags.set(MessageFlag::SharedMessage);
        }

        sink.addMessage(msg, MessageContext::Original);
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto msg = parseNoticeMessage(message);

    QString channelName;
    if (!trimChannelName(message->target(), channelName) ||
        channelName == "jtv")
    {
        // Notice wasn't targeted at a single channel, send to all twitch
        // channels
        getApp()->getTwitch()->forEachChannelAndSpecialChannels(
            [msg](const auto &c) {
                c->addMessage(msg, MessageContext::Original);
            });

        return;
    }

    auto channel = getApp()->getTwitch()->getChannelOrEmpty(channelName);

    if (channel->isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "[IrcManager:handleNoticeMessage] Channel" << channelName
            << "not found in channel manager";
        return;
    }

    QString tags = message->tags().value("msg-id").toString();
    if (tags == "usage_delete")
    {
        channel->addSystemMessage(
            "Usage: /delete <msg-id> - Deletes the specified message. "
            "Can't take more than one argument.");
    }
    else if (tags == "bad_delete_message_error")
    {
        channel->addSystemMessage(
            "There was a problem deleting the message. "
            "It might be from another channel or too old to delete.");
    }
    else if (tags == "host_on" || tags == "host_target_went_offline")
    {
        bool hostOn = (tags == "host_on");
        QStringList parts = msg->messageText.split(QLatin1Char(' '));
        if ((hostOn && parts.size() != 3) || (!hostOn && parts.size() != 7))
        {
            return;
        }
        auto &hostedChannelName = hostOn ? parts[2] : parts[0];
        if (hostedChannelName.size() < 2)
        {
            return;
        }
        if (hostOn)
        {
            hostedChannelName.chop(1);
        }
        channel->addMessage(
            MessageBuilder::makeHostingSystemMessage(hostedChannelName, hostOn),
            MessageContext::Original);
    }
    else if (tags == "room_mods" || tags == "vips_success")
    {
        // /mods and /vips
        // room_mods: The moderators of this channel are: ampzyh, antichriststollen, apa420, ...
        // vips_success: The VIPs of this channel are: 8008, aiden, botfactory, ...

        QString noticeText = msg->messageText;
        if (tags == "vips_success")
        {
            // this one has a trailing period, need to get rid of it.
            noticeText.chop(1);
        }

        QStringList msgParts = noticeText.split(':');
        MessageBuilder builder;

        auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
        assert(tc != nullptr &&
               "IrcMessageHandler::handleNoticeMessage. Twitch specific "
               "functionality called in non twitch channel");

        auto users = msgParts.at(1)
                         .mid(1)  // there is a space before the first user
                         .split(", ");
        users.sort(Qt::CaseInsensitive);
        channel->addMessage(
            MessageBuilder::makeListOfUsersMessage(msgParts.at(0), users, tc),
            MessageContext::Original);
    }
    else
    {
        channel->addMessage(msg, MessageContext::Original);
    }
}

void IrcMessageHandler::handleJoinMessage(Communi::IrcMessage *message)
{
    auto channel = getApp()->getTwitch()->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (!twitchChannel)
    {
        return;
    }

    if (message->nick() ==
        getApp()->getAccounts()->twitch.getCurrent()->getUserName())
    {
        twitchChannel->addSystemMessage("joined channel");
        twitchChannel->joined.invoke();
    }
    else if (getSettings()->showJoins.getValue())
    {
        twitchChannel->addJoinedUser(message->nick());
    }
}

void IrcMessageHandler::handlePartMessage(Communi::IrcMessage *message)
{
    auto channel = getApp()->getTwitch()->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (!twitchChannel)
    {
        return;
    }

    const auto selfAccountName =
        getApp()->getAccounts()->twitch.getCurrent()->getUserName();
    if (message->nick() != selfAccountName &&
        getSettings()->showParts.getValue())
    {
        twitchChannel->addPartedUser(message->nick());
    }

    if (message->nick() == selfAccountName)
    {
        channel->addMessage(generateBannedMessage(false),
                            MessageContext::Original);
    }
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *message,
                                   MessageSink &sink, TwitchChannel *chan,
                                   const QString &originalContent,
                                   ITwitchIrcServer &twitch, bool isSub,
                                   bool isAction)
{
    assert(chan);

    MessageParseArgs args;
    if (isSub)
    {
        args.isSubscriptionMessage = true;
        args.trimSubscriberUsername = true;
    }

    if (chan->isBroadcaster())
    {
        args.isStaffOrBroadcaster = true;
    }
    args.isAction = isAction;

    const auto &tags = message->tags();
    QString rewardId;
    if (const auto it = tags.find("custom-reward-id"); it != tags.end())
    {
        rewardId = it.value().toString();
    }
    else if (const auto typeIt = tags.find("msg-id"); typeIt != tags.end())
    {
        // slight hack to treat bits power-ups as channel point redemptions
        const auto msgId = typeIt.value().toString();
        if (msgId == "animated-message" || msgId == "gigantified-emote-message")
        {
            rewardId = msgId;
        }
    }
    if (!rewardId.isEmpty() &&
        sink.sinkTraits().has(
            MessageSinkTrait::RequiresKnownChannelPointReward) &&
        !chan->isChannelPointRewardKnown(rewardId))
    {
        // Need to wait for pubsub reward notification
        qCDebug(chatterinoTwitch) << "TwitchChannel reward added ADD "
                                     "callback since reward is not known:"
                                  << rewardId;
        chan->addQueuedRedemption(rewardId, originalContent, message);
    }
    args.channelPointRewardId = rewardId;

    QString content = originalContent;
    int messageOffset = stripLeadingReplyMention(tags, content);

    ReplyContext replyCtx;

    if (const auto it = tags.find("reply-thread-parent-msg-id");
        it != tags.end())
    {
        const QString replyID = it.value().toString();
        auto threadIt = chan->threads().find(replyID);
        std::shared_ptr<MessageThread> rootThread;
        if (threadIt != chan->threads().end() && !threadIt->second.expired())
        {
            // Thread already exists (has a reply)
            auto thread = threadIt->second.lock();
            checkThreadSubscription(tags, message->nick(), thread);
            replyCtx.thread = thread;
            rootThread = thread;
        }
        else
        {
            // Thread does not yet exist, find root reply and create thread.
            auto root = sink.findMessageByID(replyID);
            if (root)
            {
                // Found root reply message
                auto newThread = std::make_shared<MessageThread>(root);
                checkThreadSubscription(tags, message->nick(), newThread);

                replyCtx.thread = newThread;
                rootThread = newThread;
                // Store weak reference to thread in channel
                chan->addReplyThread(newThread);
            }
        }

        if (const auto parentIt = tags.find("reply-parent-msg-id");
            parentIt != tags.end())
        {
            const QString parentID = parentIt.value().toString();
            if (replyID == parentID)
            {
                if (rootThread)
                {
                    replyCtx.parent = rootThread->root();
                }
            }
            else
            {
                auto parentThreadIt = chan->threads().find(parentID);
                if (parentThreadIt != chan->threads().end())
                {
                    auto thread = parentThreadIt->second.lock();
                    if (thread)
                    {
                        replyCtx.parent = thread->root();
                    }
                }
                else
                {
                    auto parent = sink.findMessageByID(parentID);
                    if (parent)
                    {
                        replyCtx.parent = parent;
                    }
                }
            }
        }
    }

    args.allowIgnore = !isSub;
    auto [msg, alert] = MessageBuilder::makeIrcMessage(
        chan, message, args, content, messageOffset, replyCtx.thread,
        replyCtx.parent);

    if (msg)
    {
        if (isSub)
        {
            msg->flags.set(MessageFlag::Subscription);

            if (tags.value("msg-id") != "announcement")
            {
                // Announcements are currently tagged as subscriptions,
                // but we want them to be able to show up in mentions
                msg->flags.unset(MessageFlag::Highlighted);
            }
        }

        sink.applySimilarityFilters(msg);

        if (!msg->flags.has(MessageFlag::Similar) ||
            (!getSettings()->hideSimilar &&
             getSettings()->shownSimilarTriggerHighlights))
        {
            MessageBuilder::triggerHighlights(chan, alert);
        }

        const auto highlighted = msg->flags.has(MessageFlag::Highlighted);
        const auto showInMentions = msg->flags.has(MessageFlag::ShowInMentions);

        if (highlighted && showInMentions &&
            sink.sinkTraits().has(MessageSinkTrait::AddMentionsToGlobalChannel))
        {
            twitch.getMentionsChannel()->addMessage(msg,
                                                    MessageContext::Original);
        }

        sink.addMessage(msg, MessageContext::Original);
        chan->addRecentChatter(msg->displayName);
    }
}

}  // namespace chatterino
