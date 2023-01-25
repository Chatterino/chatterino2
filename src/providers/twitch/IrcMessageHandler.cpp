#include "IrcMessageHandler.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
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
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"
#include "util/StreamerMode.hpp"

#include <IrcMessage>

#include <memory>
#include <unordered_set>

namespace {
using namespace chatterino;

// Message types below are the ones that might contain special user's message on USERNOTICE
static const QSet<QString> specialMessageTypes{
    "sub",            //
    "subgift",        //
    "resub",          // resub messages
    "bitsbadgetier",  // bits badge upgrade
    "ritual",         // new viewer ritual
    "announcement",   // new mod announcement thing
};

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
        getApp()->accounts->twitch.getCurrent()->getUserName();
    if (thread->participated())
    {
        builder.message().flags.set(MessageFlag::ParticipatedThread);
        return;
    }

    if (isNew)
    {
        if (const auto it = tags.find("reply-parent-user-login");
            it != tags.end())
        {
            auto name = it.value().toString();
            if (name == currentLogin)
            {
                thread->markParticipated();
                builder.message().flags.set(MessageFlag::ParticipatedThread);
                return;  // already marked as participated
            }
        }
    }

    if (senderLogin == currentLogin)
    {
        thread->markParticipated();
        // don't set the highlight here
    }
}

}  // namespace
namespace chatterino {

static float relativeSimilarity(const QString &str1, const QString &str2)
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
    return z == 0 ? 0.f
                  : float(z) /
                        std::max<int>(1, std::max(str1.size(), str2.size()));
};

float IrcMessageHandler::similarity(
    MessagePtr msg, const LimitedQueueSnapshot<MessagePtr> &messages)
{
    float similarityPercent = 0.0f;
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

void IrcMessageHandler::setSimilarityFlags(MessagePtr msg, ChannelPtr chan)
{
    if (getSettings()->similarityEnabled)
    {
        bool isMyself = msg->loginName ==
                        getApp()->accounts->twitch.getCurrent()->getUserName();
        bool hideMyself = getSettings()->hideSimilarMyself;

        if (isMyself && !hideMyself)
        {
            return;
        }

        if (IrcMessageHandler::similarity(msg, chan->getMessageSnapshot()) >
            getSettings()->similarityPercentage)
        {
            msg->flags.set(MessageFlag::Similar, true);
            if (getSettings()->colorSimilarDisabled)
            {
                msg->flags.set(MessageFlag::Disabled, true);
            }
        }
    }
}

static QMap<QString, QString> parseBadges(QString badgesString)
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

IrcMessageHandler &IrcMessageHandler::instance()
{
    static IrcMessageHandler instance;
    return instance;
}

std::vector<MessagePtr> IrcMessageHandler::parseMessage(
    Channel *channel, Communi::IrcMessage *message)
{
    std::vector<MessagePtr> builtMessages;

    auto command = message->command();

    if (command == "PRIVMSG")
    {
        return this->parsePrivMessage(
            channel, static_cast<Communi::IrcPrivateMessage *>(message));
    }
    else if (command == "USERNOTICE")
    {
        return this->parseUserNoticeMessage(channel, message);
    }
    else if (command == "NOTICE")
    {
        return this->parseNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }

    return builtMessages;
}

std::vector<MessagePtr> IrcMessageHandler::parsePrivMessage(
    Channel *channel, Communi::IrcPrivateMessage *message)
{
    std::vector<MessagePtr> builtMessages;
    MessageParseArgs args;
    TwitchMessageBuilder builder(channel, message, args, message->content(),
                                 message->isAction());
    if (!builder.isIgnored())
    {
        builtMessages.emplace_back(builder.build());
        builder.triggerHighlights();
    }
    return builtMessages;
}

void IrcMessageHandler::handlePrivMessage(Communi::IrcPrivateMessage *message,
                                          TwitchIrcServer &server)
{
    // This is to make sure that combined emoji go through properly, see
    // https://github.com/Chatterino/chatterino2/issues/3384 and
    // https://mm2pl.github.io/emoji_rfc.pdf for more details
    // Constants used here are defined in TwitchChannel.hpp

    this->addMessage(
        message, message->target(),
        message->content().replace(COMBINED_FIXER, ZERO_WIDTH_JOINER), server,
        false, message->isAction());
}

std::vector<MessagePtr> IrcMessageHandler::parseMessageWithReply(
    Channel *channel, Communi::IrcMessage *message,
    const std::vector<MessagePtr> &otherLoaded)
{
    std::vector<MessagePtr> builtMessages;

    auto command = message->command();

    if (command == "PRIVMSG")
    {
        auto privMsg = static_cast<Communi::IrcPrivateMessage *>(message);
        auto tc = dynamic_cast<TwitchChannel *>(channel);
        if (!tc)
        {
            return this->parsePrivMessage(channel, privMsg);
        }

        QString content = privMsg->content();
        int messageOffset = stripLeadingReplyMention(privMsg->tags(), content);
        MessageParseArgs args;
        TwitchMessageBuilder builder(channel, message, args, content,
                                     privMsg->isAction());
        builder.setMessageOffset(messageOffset);

        this->populateReply(tc, message, otherLoaded, builder);

        if (!builder.isIgnored())
        {
            builtMessages.emplace_back(builder.build());
            builder.triggerHighlights();
        }
    }
    else if (command == "USERNOTICE")
    {
        return this->parseUserNoticeMessage(channel, message);
    }
    else if (command == "NOTICE")
    {
        return this->parseNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }

    return builtMessages;
}

void IrcMessageHandler::populateReply(
    TwitchChannel *channel, Communi::IrcMessage *message,
    const std::vector<MessagePtr> &otherLoaded, TwitchMessageBuilder &builder)
{
    const auto &tags = message->tags();
    if (const auto it = tags.find("reply-parent-msg-id"); it != tags.end())
    {
        const QString replyID = it.value().toString();
        auto threadIt = channel->threads_.find(replyID);
        if (threadIt != channel->threads_.end())
        {
            auto owned = threadIt->second.lock();
            if (owned)
            {
                // Thread already exists (has a reply)
                updateReplyParticipatedStatus(tags, message->nick(), builder,
                                              owned, false);
                builder.setThread(owned);
                return;
            }
        }

        MessagePtr foundMessage;

        // Thread does not yet exist, find root reply and create thread.
        // Linear search is justified by the infrequent use of replies
        for (auto &otherMsg : otherLoaded)
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
            // Store weak reference to thread in channel
            channel->addReplyThread(newThread);
        }
    }
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *_message,
                                   const QString &target,
                                   const QString &content_,
                                   TwitchIrcServer &server, bool isSub,
                                   bool isAction)
{
    QString channelName;
    if (!trimChannelName(target, channelName))
    {
        return;
    }

    auto chan = server.getChannelOrEmpty(channelName);

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

    auto channel = dynamic_cast<TwitchChannel *>(chan.get());

    const auto &tags = _message->tags();
    if (const auto it = tags.find("custom-reward-id"); it != tags.end())
    {
        const auto rewardId = it.value().toString();
        if (!channel->isChannelPointRewardKnown(rewardId))
        {
            // Need to wait for pubsub reward notification
            auto clone = _message->clone();
            qCDebug(chatterinoTwitch) << "TwitchChannel reward added ADD "
                                         "callback since reward is not known:"
                                      << rewardId;
            channel->channelPointRewardAdded.connect(
                [=, this, &server](ChannelPointReward reward) {
                    qCDebug(chatterinoTwitch)
                        << "TwitchChannel reward added callback:" << reward.id
                        << "-" << rewardId;
                    if (reward.id == rewardId)
                    {
                        this->addMessage(clone, target, content_, server, isSub,
                                         isAction);
                        clone->deleteLater();
                        return true;
                    }
                    return false;
                });
            return;
        }
        args.channelPointRewardId = rewardId;
    }

    QString content = content_;
    int messageOffset = stripLeadingReplyMention(tags, content);

    TwitchMessageBuilder builder(chan.get(), _message, args, content, isAction);
    builder.setMessageOffset(messageOffset);

    if (const auto it = tags.find("reply-parent-msg-id"); it != tags.end())
    {
        const QString replyID = it.value().toString();
        auto threadIt = channel->threads_.find(replyID);
        if (threadIt != channel->threads_.end() && !threadIt->second.expired())
        {
            // Thread already exists (has a reply)
            auto thread = threadIt->second.lock();
            updateReplyParticipatedStatus(tags, _message->nick(), builder,
                                          thread, false);
            builder.setThread(thread);
        }
        else
        {
            // Thread does not yet exist, find root reply and create thread.
            auto root = channel->findMessage(replyID);
            if (root)
            {
                // Found root reply message
                auto newThread = std::make_shared<MessageThread>(root);
                updateReplyParticipatedStatus(tags, _message->nick(), builder,
                                              newThread, true);

                builder.setThread(newThread);
                // Store weak reference to thread in channel
                channel->addReplyThread(newThread);
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
            server.mentionsChannel->addMessage(msg);
        }

        chan->addMessage(msg);
        if (auto chatters = dynamic_cast<ChannelChatters *>(chan.get()))
        {
            chatters->addRecentChatter(msg->displayName);
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
            << "[IrcMessageHandler:handleClearChatMessage] Twitch channel"
            << chanName << "not found";
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1)
    {
        chan->disableAllMessages();
        chan->addMessage(
            makeSystemMessage("Chat has been cleared by a moderator.",
                              calculateMessageTime(message).time()));

        return;
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
    chan->addOrReplaceTimeout(timeoutMsg);

    // refresh all
    getApp()->windows->repaintVisibleChatWidgets(chan.get());
    if (getSettings()->hideModerated)
    {
        getApp()->windows->forceLayoutChannelViews();
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
        return;

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
    auto currentUser = getApp()->accounts->twitch.getCurrent();

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

    // parallel universe
    auto pu = getApp()->twitch->getChannelOrEmpty("$" + channelName);
    if (c->isEmpty() && pu->isEmpty())
    {
        return;
    }

    // Checking if currentUser is a VIP or staff member
    QVariant _badges = message->tag("badges");
    if (_badges.isValid())
    {
        if (!c->isEmpty())
        {
            TwitchChannel *tc = dynamic_cast<TwitchChannel *>(c.get());
            if (tc != nullptr)
            {
                auto parsedBadges = parseBadges(_badges.toString());
                tc->setVIP(parsedBadges.contains("vip"));
                tc->setStaff(parsedBadges.contains("staff"));
            }
        }
        if (!pu->isEmpty())
        {
            TwitchChannel *putc = dynamic_cast<TwitchChannel *>(pu.get());
            if (putc != nullptr)
            {
                auto parsedBadges = parseBadges(_badges.toString());
                putc->setVIP(parsedBadges.contains("vip"));
                putc->setStaff(parsedBadges.contains("staff"));
            }
        }
    }

    // Checking if currentUser is a moderator
    QVariant _mod = message->tag("mod");
    if (_mod.isValid())
    {
        if (!c->isEmpty())
        {
            TwitchChannel *tc = dynamic_cast<TwitchChannel *>(c.get());
            if (tc != nullptr)
            {
                tc->setMod(_mod == "1");
            }
        }
        if (!pu->isEmpty())
        {
            TwitchChannel *putc = dynamic_cast<TwitchChannel *>(pu.get());
            if (putc != nullptr)
            {
                putc->setMod(_mod == "1");
            }
        }
    }
}

// This will emit only once and right after user logs in to IRC - reset emote data and reload emotes
void IrcMessageHandler::handleGlobalUserStateMessage(
    Communi::IrcMessage *message)
{
    auto currentUser = getApp()->accounts->twitch.getCurrent();

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

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *message)
{
    MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto c = getApp()->twitch->whispersChannel.get();

    TwitchMessageBuilder builder(
        c, message, args,
        message->parameter(1).replace(COMBINED_FIXER, ZERO_WIDTH_JOINER),
        false);

    if (builder.isIgnored())
    {
        return;
    }

    builder->flags.set(MessageFlag::Whisper);
    MessagePtr _message = builder.build();
    builder.triggerHighlights();

    getApp()->twitch->lastUserThatWhisperedMe.set(builder.userName);

    if (_message->flags.has(MessageFlag::Highlighted))
    {
        getApp()->twitch->mentionsChannel->addMessage(_message);
    }

    c->addMessage(_message);

    auto overrideFlags = boost::optional<MessageFlags>(_message->flags);
    overrideFlags->set(MessageFlag::DoNotTriggerNotification);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers &&
        !(getSettings()->streamerModeSuppressInlineWhispers &&
          isInStreamerMode()))
    {
        getApp()->twitch->forEachChannel(
            [&_message, overrideFlags](ChannelPtr channel) {
                channel->addMessage(_message, overrideFlags);
            });
    }
}

std::vector<MessagePtr> IrcMessageHandler::parseUserNoticeMessage(
    Channel *channel, Communi::IrcMessage *message)
{
    std::vector<MessagePtr> builtMessages;

    auto tags = message->tags();
    auto parameters = message->parameters();

    QString msgType = tags.value("msg-id").toString();
    QString content;
    if (parameters.size() >= 2)
    {
        content = parameters[1];
    }

    if (specialMessageTypes.contains(msgType))
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

        auto b = MessageBuilder(systemMessage, parseTagString(messageText),
                                calculateMessageTime(message).time());

        b->flags.set(MessageFlag::Subscription);
        auto newMessage = b.release();
        builtMessages.emplace_back(newMessage);
    }

    return builtMessages;
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

    if (specialMessageTypes.contains(msgType))
    {
        // Messages are not required, so they might be empty
        if (!content.isEmpty())
        {
            this->addMessage(message, target, content, server, true, false);
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

std::vector<MessagePtr> IrcMessageHandler::parseNoticeMessage(
    Communi::IrcNoticeMessage *message)
{
    if (message->content().startsWith("Login auth", Qt::CaseInsensitive))
    {
        const auto linkColor = MessageColor(MessageColor::Link);
        const auto accountsLink = Link(Link::OpenAccountsPage, QString());
        const auto curUser = getApp()->accounts->twitch.getCurrent();
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
    else if (message->content().startsWith("You are permanently banned "))
    {
        return {generateBannedMessage(true)};
    }
    else if (message->tags().value("msg-id") == "msg_timedout")
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

    auto content = message->content();
    if (content.startsWith(
            "Your settings prevent you from sending this whisper",
            Qt::CaseInsensitive) &&
        getSettings()->helixTimegateWhisper.getValue() ==
            HelixTimegateOverride::Timegate)
    {
        content = content +
                  " Consider setting \"Helix timegate /w behaviour\" "
                  "to \"Always use Helix\" in your Chatterino settings.";
    }
    builtMessages.emplace_back(
        makeSystemMessage(content, calculateMessageTime(message).time()));

    return builtMessages;
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto builtMessages = this->parseNoticeMessage(message);

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

            auto tc = dynamic_cast<TwitchChannel *>(channel.get());
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

    if (message->nick() !=
            getApp()->accounts->twitch.getCurrent()->getUserName() &&
        getSettings()->showJoins.getValue())
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
        getApp()->accounts->twitch.getCurrent()->getUserName();
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
}  // namespace chatterino
