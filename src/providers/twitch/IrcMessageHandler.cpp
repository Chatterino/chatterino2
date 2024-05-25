#include "providers/twitch/IrcMessageHandler.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/Link.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/WindowManager.hpp"
#include "util/ChannelHelpers.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"

#include <IrcMessage>
#include <QLocale>
#include <QStringBuilder>

#include <memory>
#include <unordered_set>

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

void updateReplyParticipatedStatus(const QVariantMap &tags,
                                   const QString &senderLogin,
                                   TwitchMessageBuilder &builder,
                                   std::shared_ptr<MessageThread> &thread,
                                   bool isNew)
{
    const auto &currentLogin =
        getIApp()->getAccounts()->twitch.getCurrent()->getUserName();

    if (thread->subscribed())
    {
        builder.message().flags.set(MessageFlag::SubscribedThread);
        return;
    }

    if (thread->unsubscribed())
    {
        return;
    }

    if (getSettings()->autoSubToParticipatedThreads)
    {
        if (isNew)
        {
            if (const auto it = tags.find("reply-parent-user-login");
                it != tags.end())
            {
                auto name = it.value().toString();
                if (name == currentLogin)
                {
                    thread->markSubscribed();
                    builder.message().flags.set(MessageFlag::SubscribedThread);
                    return;  // already marked as participated
                }
            }
        }

        if (senderLogin == currentLogin)
        {
            thread->markSubscribed();
            // don't set the highlight here
        }
    }
}

ChannelPtr channelOrEmptyByTarget(const QString &target,
                                  TwitchIrcServer &server)
{
    QString channelName;
    if (!trimChannelName(target, channelName))
    {
        return Channel::getEmpty();
    }

    return server.getChannelOrEmpty(channelName);
}

float relativeSimilarity(const QString &str1, const QString &str2)
{
    // Longest Common Substring Problem
    std::vector<std::vector<int>> tree(str1.size(),
                                       std::vector<int>(str2.size(), 0));
    int z = 0;

    for (int i = 0; i < str1.size(); ++i)
    {
        for (int j = 0; j < str2.size(); ++j)
        {
            if (str1[i] == str2[j])
            {
                if (i == 0 || j == 0)
                {
                    tree[i][j] = 1;
                }
                else
                {
                    tree[i][j] = tree[i - 1][j - 1] + 1;
                }
                if (tree[i][j] > z)
                {
                    z = tree[i][j];
                }
            }
            else
            {
                tree[i][j] = 0;
            }
        }
    }

    // ensure that no div by 0
    if (z == 0)
    {
        return 0.F;
    }

    auto div = std::max<int>(1, std::max(str1.size(), str2.size()));

    return float(z) / float(div);
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

void populateReply(TwitchChannel *channel, Communi::IrcMessage *message,
                   const std::vector<MessagePtr> &otherLoaded,
                   TwitchMessageBuilder &builder)
{
    const auto &tags = message->tags();
    if (const auto it = tags.find("reply-thread-parent-msg-id");
        it != tags.end())
    {
        const QString replyID = it.value().toString();
        auto threadIt = channel->threads().find(replyID);
        std::shared_ptr<MessageThread> rootThread;
        if (threadIt != channel->threads().end())
        {
            auto owned = threadIt->second.lock();
            if (owned)
            {
                // Thread already exists (has a reply)
                updateReplyParticipatedStatus(tags, message->nick(), builder,
                                              owned, false);
                builder.setThread(owned);
                rootThread = owned;
            }
        }

        if (!rootThread)
        {
            MessagePtr foundMessage;

            // Thread does not yet exist, find root reply and create thread.
            // Linear search is justified by the infrequent use of replies
            for (const auto &otherMsg : otherLoaded)
            {
                if (otherMsg->id == replyID)
                {
                    // Found root reply message
                    foundMessage = otherMsg;
                    break;
                }
            }

            if (!foundMessage)
            {
                // We didn't find the reply root message in the otherLoaded messages
                // which are typically the already-parsed recent messages from the
                // Recent Messages API. We could have a really old message that
                // still exists being replied to, so check for that here.
                foundMessage = channel->findMessage(replyID);
            }

            if (foundMessage)
            {
                std::shared_ptr<MessageThread> newThread =
                    std::make_shared<MessageThread>(foundMessage);
                updateReplyParticipatedStatus(tags, message->nick(), builder,
                                              newThread, true);

                builder.setThread(newThread);
                rootThread = newThread;
                // Store weak reference to thread in channel
                channel->addReplyThread(newThread);
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
                    builder.setParent(rootThread->root());
                }
            }
            else
            {
                auto parentThreadIt = channel->threads().find(parentID);
                if (parentThreadIt != channel->threads().end())
                {
                    auto thread = parentThreadIt->second.lock();
                    if (thread)
                    {
                        builder.setParent(thread->root());
                    }
                }
                else
                {
                    auto parent = channel->findMessage(parentID);
                    if (parent)
                    {
                        builder.setParent(parent);
                    }
                }
            }
        }
    }
}

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
            .message =
                makeSystemMessage("Chat has been cleared by a moderator.",
                                  calculateMessageTime(message).time()),
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
                       calculateMessageTime(message).time())
            .release();

    return ClearChatMessage{.message = timeoutMsg, .disableAllMessages = false};
}

/**
 * Parse a single IRC NOTICE message into 0 or more Chatterino messages
 **/
std::vector<MessagePtr> parseNoticeMessage(Communi::IrcNoticeMessage *message)
{
    assert(message != nullptr);

    if (message->content().startsWith("Login auth", Qt::CaseInsensitive))
    {
        const auto linkColor = MessageColor(MessageColor::Link);
        const auto accountsLink = Link(Link::OpenAccountsPage, QString());
        const auto curUser = getIApp()->getAccounts()->twitch.getCurrent();
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

        return {builder.release()};
    }

    if (message->content().startsWith("You are permanently banned "))
    {
        return {generateBannedMessage(true)};
    }

    if (message->tags().value("msg-id") == "msg_timedout")
    {
        std::vector<MessagePtr> builtMessage;

        QString remainingTime =
            formatTime(message->content().split(" ").value(5));
        QString formattedMessage =
            QString("You are timed out for %1.")
                .arg(remainingTime.isEmpty() ? "0s" : remainingTime);

        builtMessage.emplace_back(makeSystemMessage(
            formattedMessage, calculateMessageTime(message).time()));

        return builtMessage;
    }

    // default case
    std::vector<MessagePtr> builtMessages;

    builtMessages.emplace_back(makeSystemMessage(
        message->content(), calculateMessageTime(message).time()));

    return builtMessages;
}

/**
 * Parse a single IRC USERNOTICE message into 0 or more Chatterino messages
 **/
std::vector<MessagePtr> parseUserNoticeMessage(Channel *channel,
                                               Communi::IrcMessage *message)
{
    assert(channel != nullptr);
    assert(message != nullptr);

    std::vector<MessagePtr> builtMessages;

    auto tags = message->tags();
    auto parameters = message->parameters();

    QString msgType = tags.value("msg-id").toString();
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
        return {};
    }

    if (SPECIAL_MESSAGE_TYPES.contains(msgType))
    {
        // Messages are not required, so they might be empty
        if (!content.isEmpty())
        {
            MessageParseArgs args;
            args.trimSubscriberUsername = true;

            TwitchMessageBuilder builder(channel, message, args, content,
                                         false);
            builder->flags.set(MessageFlag::Subscription);
            builder->flags.unset(MessageFlag::Highlighted);
            builtMessages.emplace_back(builder.build());
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
        }

        auto b = MessageBuilder(systemMessage, parseTagString(messageText),
                                calculateMessageTime(message).time());

        b->flags.set(MessageFlag::Subscription);
        auto newMessage = b.release();
        builtMessages.emplace_back(newMessage);
    }

    return builtMessages;
}

/**
 * Parse a single IRC PRIVMSG into 0-1 Chatterino messages
 */
std::vector<MessagePtr> parsePrivMessage(Channel *channel,
                                         Communi::IrcPrivateMessage *message)
{
    assert(channel != nullptr);
    assert(message != nullptr);

    std::vector<MessagePtr> builtMessages;
    MessageParseArgs args;
    TwitchMessageBuilder builder(channel, message, args, message->content(),
                                 message->isAction());
    if (!builder.isIgnored())
    {
        builtMessages.emplace_back(builder.build());
        builder.triggerHighlights();
    }

    if (message->tags().contains(u"pinned-chat-paid-amount"_s))
    {
        auto ptr = TwitchMessageBuilder::buildHypeChatMessage(message);
        if (ptr)
        {
            builtMessages.emplace_back(std::move(ptr));
        }
    }

    return builtMessages;
}

}  // namespace

namespace chatterino {

using namespace literals;

IrcMessageHandler &IrcMessageHandler::instance()
{
    static IrcMessageHandler instance;
    return instance;
}

std::vector<MessagePtr> IrcMessageHandler::parseMessageWithReply(
    Channel *channel, Communi::IrcMessage *message,
    std::vector<MessagePtr> &otherLoaded)
{
    std::vector<MessagePtr> builtMessages;

    auto command = message->command();

    if (command == u"PRIVMSG"_s)
    {
        auto *privMsg = dynamic_cast<Communi::IrcPrivateMessage *>(message);
        auto *tc = dynamic_cast<TwitchChannel *>(channel);
        if (!tc)
        {
            return parsePrivMessage(channel, privMsg);
        }

        QString content = privMsg->content();
        int messageOffset = stripLeadingReplyMention(privMsg->tags(), content);
        MessageParseArgs args;
        TwitchMessageBuilder builder(channel, message, args, content,
                                     privMsg->isAction());
        builder.setMessageOffset(messageOffset);

        populateReply(tc, message, otherLoaded, builder);

        if (!builder.isIgnored())
        {
            builtMessages.emplace_back(builder.build());
            builder.triggerHighlights();
        }

        return builtMessages;
    }

    if (command == u"USERNOTICE"_s)
    {
        return parseUserNoticeMessage(channel, message);
    }

    if (command == u"NOTICE"_s)
    {
        return parseNoticeMessage(
            dynamic_cast<Communi::IrcNoticeMessage *>(message));
    }

    if (command == u"CLEARCHAT"_s)
    {
        auto cc = parseClearChatMessage(message);
        if (!cc)
        {
            return builtMessages;
        }
        auto &clearChat = *cc;
        if (clearChat.disableAllMessages)
        {
            builtMessages.emplace_back(std::move(clearChat.message));
        }
        else
        {
            addOrReplaceChannelTimeout(
                otherLoaded, std::move(clearChat.message),
                calculateMessageTime(message).time(),
                [&](auto idx, auto /*msg*/, auto &&replacement) {
                    replacement->flags.set(MessageFlag::RecentMessage);
                    otherLoaded[idx] = replacement;
                },
                [&](auto &&msg) {
                    builtMessages.emplace_back(msg);
                },
                false);
        }

        return builtMessages;
    }

    return builtMessages;
}

void IrcMessageHandler::handlePrivMessage(Communi::IrcPrivateMessage *message,
                                          TwitchIrcServer &server)
{
    auto chan = channelOrEmptyByTarget(message->target(), server);
    if (chan->isEmpty())
    {
        return;
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(chan.get());

    if (twitchChannel != nullptr)
    {
        auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
        if (message->tag("user-id") == currentUser->getUserId())
        {
            auto badgesTag = message->tag("badges");
            if (badgesTag.isValid())
            {
                auto parsedBadges = parseBadges(badgesTag.toString());
                twitchChannel->setMod(parsedBadges.contains("moderator"));
                twitchChannel->setVIP(parsedBadges.contains("vip"));
                twitchChannel->setStaff(parsedBadges.contains("staff"));
            }
        }
    }

    // This is for compatibility with older Chatterino versions. Twitch didn't use
    // to allow ZERO WIDTH JOINER unicode character, so Chatterino used ESCAPE_TAG
    // instead.
    // See https://github.com/Chatterino/chatterino2/issues/3384 and
    // https://mm2pl.github.io/emoji_rfc.pdf for more details
    this->addMessage(
        message, chan,
        message->content().replace(COMBINED_FIXER, ZERO_WIDTH_JOINER), server,
        false, message->isAction());

    if (message->tags().contains(u"pinned-chat-paid-amount"_s))
    {
        auto ptr = TwitchMessageBuilder::buildHypeChatMessage(message);
        if (ptr)
        {
            chan->addMessage(ptr);
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
    auto chan = getApp()->twitch->getChannelOrEmpty(chanName);

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
    auto chan = getApp()->twitch->getChannelOrEmpty(chanName);

    if (chan->isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "[IrcMessageHandler::handleClearChatMessage] Twitch channel"
            << chanName << "not found";
        return;
    }

    // chat has been cleared by a moderator
    if (clearChat.disableAllMessages)
    {
        chan->disableAllMessages();
        chan->addMessage(std::move(clearChat.message));

        return;
    }

    chan->addOrReplaceTimeout(std::move(clearChat.message));

    // refresh all
    getIApp()->getWindows()->repaintVisibleChatWidgets(chan.get());
    if (getSettings()->hideModerated)
    {
        getIApp()->getWindows()->forceLayoutChannelViews();
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
    auto chan = getApp()->twitch->getChannelOrEmpty(chanName);

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

    auto msg = chan->findMessage(targetID);
    if (msg == nullptr)
    {
        return;
    }

    msg->flags.set(MessageFlag::Disabled);
    if (!getSettings()->hideDeletionActions)
    {
        MessageBuilder builder;
        TwitchMessageBuilder::deletionMessage(msg, &builder);
        chan->addMessage(builder.release());
    }
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();

    // set received emote-sets, used in TwitchAccount::loadUserstateEmotes
    bool emoteSetsChanged = currentUser->setUserstateEmoteSets(
        message->tag("emote-sets").toString().split(","));

    if (emoteSetsChanged)
    {
        currentUser->loadUserstateEmotes();
    }

    QString channelName;
    if (!trimChannelName(message->parameter(0), channelName))
    {
        return;
    }

    auto c = getApp()->twitch->getChannelOrEmpty(channelName);
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

// This will emit only once and right after user logs in to IRC - reset emote data and reload emotes
void IrcMessageHandler::handleGlobalUserStateMessage(
    Communi::IrcMessage *message)
{
    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();

    // set received emote-sets, this time used to initially load emotes
    // NOTE: this should always return true unless we reconnect
    auto emoteSetsChanged = currentUser->setUserstateEmoteSets(
        message->tag("emote-sets").toString().split(","));

    // We should always attempt to reload emotes even on reconnections where
    // emoteSetsChanged, since we want to trigger emote reloads when
    // "currentUserChanged" signal is emitted
    qCDebug(chatterinoTwitch) << emoteSetsChanged << message->toData();
    currentUser->loadEmotes();
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *ircMessage)
{
    MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto *c = getIApp()->getTwitch()->getWhispersChannel().get();

    TwitchMessageBuilder builder(
        c, ircMessage, args,
        ircMessage->parameter(1).replace(COMBINED_FIXER, ZERO_WIDTH_JOINER),
        false);

    if (builder.isIgnored())
    {
        return;
    }

    builder->flags.set(MessageFlag::Whisper);
    MessagePtr message = builder.build();
    builder.triggerHighlights();

    getApp()->twitch->lastUserThatWhisperedMe.set(builder.userName);

    if (message->flags.has(MessageFlag::ShowInMentions))
    {
        getIApp()->getTwitch()->getMentionsChannel()->addMessage(message);
    }

    c->addMessage(message);

    auto overrideFlags = std::optional<MessageFlags>(message->flags);
    overrideFlags->set(MessageFlag::DoNotTriggerNotification);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers &&
        !(getSettings()->streamerModeSuppressInlineWhispers &&
          getIApp()->getStreamerMode()->isEnabled()))
    {
        getApp()->twitch->forEachChannel(
            [&message, overrideFlags](ChannelPtr channel) {
                channel->addMessage(message, overrideFlags);
            });
    }
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message,
                                                TwitchIrcServer &server)
{
    auto tags = message->tags();
    auto parameters = message->parameters();

    auto target = parameters[0];
    QString msgType = tags.value("msg-id").toString();
    QString content;
    if (parameters.size() >= 2)
    {
        content = parameters[1];
    }

    auto chn = server.getChannelOrEmpty(target);
    if (isIgnoredMessage({
            .message = content,
            .twitchUserID = tags.value("user-id").toString(),
            .isMod = chn->isMod(),
            .isBroadcaster = chn->isBroadcaster(),
        }))
    {
        return;
    }

    if (SPECIAL_MESSAGE_TYPES.contains(msgType))
    {
        // Messages are not required, so they might be empty
        if (!content.isEmpty())
        {
            this->addMessage(message, chn, content, server, true, false);
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
        }

        auto b = MessageBuilder(systemMessage, parseTagString(messageText),
                                calculateMessageTime(message).time());

        b->flags.set(MessageFlag::Subscription);
        auto newMessage = b.release();

        QString channelName;

        if (message->parameters().size() < 1)
        {
            return;
        }

        if (!trimChannelName(message->parameter(0), channelName))
        {
            return;
        }

        auto chan = server.getChannelOrEmpty(channelName);

        if (!chan->isEmpty())
        {
            chan->addMessage(newMessage);
        }
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto builtMessages = parseNoticeMessage(message);

    for (const auto &msg : builtMessages)
    {
        QString channelName;
        if (!trimChannelName(message->target(), channelName) ||
            channelName == "jtv")
        {
            // Notice wasn't targeted at a single channel, send to all twitch
            // channels
            getApp()->twitch->forEachChannelAndSpecialChannels(
                [msg](const auto &c) {
                    c->addMessage(msg);
                });

            return;
        }

        auto channel = getApp()->twitch->getChannelOrEmpty(channelName);

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
            channel->addMessage(makeSystemMessage(
                "Usage: /delete <msg-id> - Deletes the specified message. "
                "Can't take more than one argument."));
        }
        else if (tags == "bad_delete_message_error")
        {
            channel->addMessage(makeSystemMessage(
                "There was a problem deleting the message. "
                "It might be from another channel or too old to delete."));
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
            MessageBuilder builder;
            TwitchMessageBuilder::hostingSystemMessage(hostedChannelName,
                                                       &builder, hostOn);
            channel->addMessage(builder.release());
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
            TwitchMessageBuilder::listOfUsersSystemMessage(msgParts.at(0),
                                                           users, tc, &builder);
            channel->addMessage(builder.release());
        }
        else
        {
            channel->addMessage(msg);
        }
    }
}

void IrcMessageHandler::handleJoinMessage(Communi::IrcMessage *message)
{
    auto channel =
        getApp()->twitch->getChannelOrEmpty(message->parameter(0).remove(0, 1));

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (!twitchChannel)
    {
        return;
    }

    if (message->nick() ==
        getIApp()->getAccounts()->twitch.getCurrent()->getUserName())
    {
        twitchChannel->addMessage(makeSystemMessage("joined channel"));
        twitchChannel->joined.invoke();
    }
    else if (getSettings()->showJoins.getValue())
    {
        twitchChannel->addJoinedUser(message->nick());
    }
}

void IrcMessageHandler::handlePartMessage(Communi::IrcMessage *message)
{
    auto channel =
        getApp()->twitch->getChannelOrEmpty(message->parameter(0).remove(0, 1));

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (!twitchChannel)
    {
        return;
    }

    const auto selfAccountName =
        getIApp()->getAccounts()->twitch.getCurrent()->getUserName();
    if (message->nick() != selfAccountName &&
        getSettings()->showParts.getValue())
    {
        twitchChannel->addPartedUser(message->nick());
    }

    if (message->nick() == selfAccountName)
    {
        channel->addMessage(generateBannedMessage(false));
    }
}

float IrcMessageHandler::similarity(
    const MessagePtr &msg, const LimitedQueueSnapshot<MessagePtr> &messages)
{
    float similarityPercent = 0.0F;
    int checked = 0;

    for (int i = 1; i <= messages.size(); ++i)
    {
        if (checked >= getSettings()->hideSimilarMaxMessagesToCheck)
        {
            break;
        }
        const auto &prevMsg = messages[messages.size() - i];
        if (prevMsg->parseTime.secsTo(QTime::currentTime()) >=
            getSettings()->hideSimilarMaxDelay)
        {
            break;
        }
        if (getSettings()->hideSimilarBySameUser &&
            msg->loginName != prevMsg->loginName)
        {
            continue;
        }
        ++checked;
        similarityPercent = std::max(
            similarityPercent,
            relativeSimilarity(msg->messageText, prevMsg->messageText));
    }

    return similarityPercent;
}

void IrcMessageHandler::setSimilarityFlags(const MessagePtr &message,
                                           const ChannelPtr &channel)
{
    if (getSettings()->similarityEnabled)
    {
        bool isMyself =
            message->loginName ==
            getIApp()->getAccounts()->twitch.getCurrent()->getUserName();
        bool hideMyself = getSettings()->hideSimilarMyself;

        if (isMyself && !hideMyself)
        {
            return;
        }

        if (IrcMessageHandler::similarity(message,
                                          channel->getMessageSnapshot()) >
            getSettings()->similarityPercentage)
        {
            message->flags.set(MessageFlag::Similar, true);
            if (getSettings()->colorSimilarDisabled)
            {
                message->flags.set(MessageFlag::Disabled, true);
            }
        }
    }
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *message,
                                   const ChannelPtr &chan,
                                   const QString &originalContent,
                                   TwitchIrcServer &server, bool isSub,
                                   bool isAction)
{
    if (chan->isEmpty())
    {
        return;
    }

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

    auto *channel = dynamic_cast<TwitchChannel *>(chan.get());

    const auto &tags = message->tags();
    if (const auto it = tags.find("custom-reward-id"); it != tags.end())
    {
        const auto rewardId = it.value().toString();
        if (!rewardId.isEmpty() &&
            !channel->isChannelPointRewardKnown(rewardId))
        {
            // Need to wait for pubsub reward notification
            qCDebug(chatterinoTwitch) << "TwitchChannel reward added ADD "
                                         "callback since reward is not known:"
                                      << rewardId;
            channel->addQueuedRedemption(rewardId, originalContent, message);
            return;
        }
        args.channelPointRewardId = rewardId;
    }

    QString content = originalContent;
    int messageOffset = stripLeadingReplyMention(tags, content);

    TwitchMessageBuilder builder(channel, message, args, content, isAction);
    builder.setMessageOffset(messageOffset);

    if (const auto it = tags.find("reply-thread-parent-msg-id");
        it != tags.end())
    {
        const QString replyID = it.value().toString();
        auto threadIt = channel->threads().find(replyID);
        std::shared_ptr<MessageThread> rootThread;
        if (threadIt != channel->threads().end() && !threadIt->second.expired())
        {
            // Thread already exists (has a reply)
            auto thread = threadIt->second.lock();
            updateReplyParticipatedStatus(tags, message->nick(), builder,
                                          thread, false);
            builder.setThread(thread);
            rootThread = thread;
        }
        else
        {
            // Thread does not yet exist, find root reply and create thread.
            auto root = channel->findMessage(replyID);
            if (root)
            {
                // Found root reply message
                auto newThread = std::make_shared<MessageThread>(root);
                updateReplyParticipatedStatus(tags, message->nick(), builder,
                                              newThread, true);

                builder.setThread(newThread);
                rootThread = newThread;
                // Store weak reference to thread in channel
                channel->addReplyThread(newThread);
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
                    builder.setParent(rootThread->root());
                }
            }
            else
            {
                auto parentThreadIt = channel->threads().find(parentID);
                if (parentThreadIt != channel->threads().end())
                {
                    auto thread = parentThreadIt->second.lock();
                    if (thread)
                    {
                        builder.setParent(thread->root());
                    }
                }
                else
                {
                    auto parent = channel->findMessage(parentID);
                    if (parent)
                    {
                        builder.setParent(parent);
                    }
                }
            }
        }
    }

    if (isSub || !builder.isIgnored())
    {
        if (isSub)
        {
            builder->flags.set(MessageFlag::Subscription);
            builder->flags.unset(MessageFlag::Highlighted);
        }
        auto msg = builder.build();

        IrcMessageHandler::setSimilarityFlags(msg, chan);

        if (!msg->flags.has(MessageFlag::Similar) ||
            (!getSettings()->hideSimilar &&
             getSettings()->shownSimilarTriggerHighlights))
        {
            builder.triggerHighlights();
        }

        const auto highlighted = msg->flags.has(MessageFlag::Highlighted);
        const auto showInMentions = msg->flags.has(MessageFlag::ShowInMentions);

        if (highlighted && showInMentions)
        {
            server.getMentionsChannel()->addMessage(msg);
        }

        chan->addMessage(msg);
        if (auto *chatters = dynamic_cast<ChannelChatters *>(chan.get()))
        {
            chatters->addRecentChatter(msg->displayName);
        }
    }
}

}  // namespace chatterino
