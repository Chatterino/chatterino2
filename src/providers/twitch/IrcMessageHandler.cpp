#include "IrcMessageHandler.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/Message.hpp"
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

#include <IrcMessage>

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
    this->addMessage(message, message->target(), message->content(), server,
                     false, message->isAction());
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *_message,
                                   const QString &target,
                                   const QString &content,
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
        args.trimSubscriberUsername = true;
    }

    if (chan->isBroadcaster())
    {
        args.isStaffOrBroadcaster = true;
    }

    auto channel = dynamic_cast<TwitchChannel *>(chan.get());

    const auto &tags = _message->tags();
    if (const auto &it = tags.find("custom-reward-id"); it != tags.end())
    {
        const auto rewardId = it.value().toString();
        if (!channel->isChannelPointRewardKnown(rewardId))
        {
            // Need to wait for pubsub reward notification
            auto clone = _message->clone();
            channel->channelPointRewardAdded.connect(
                [=, &server](ChannelPointReward reward) {
                    if (reward.id == rewardId)
                    {
                        this->addMessage(clone, target, content, server, isSub,
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

    TwitchMessageBuilder builder(chan.get(), _message, args, content, isAction);

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

    // get twitch channel
    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName))
    {
        return;
    }
    auto chan = getApp()->twitch.server->getChannelOrEmpty(chanName);

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
    auto chan = getApp()->twitch.server->getChannelOrEmpty(chanName);

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
                              calculateMessageTimestamp(message)));

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
                       calculateMessageTimestamp(message))
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
    auto chan = getApp()->twitch.server->getChannelOrEmpty(chanName);

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
        currentUser->loadUserstateEmotes([] {});
    }

    QString channelName;
    if (!trimChannelName(message->parameter(0), channelName))
    {
        return;
    }

    auto c = getApp()->twitch.server->getChannelOrEmpty(channelName);
    if (c->isEmpty())
    {
        return;
    }

    // Checking if currentUser is a VIP or staff member
    QVariant _badges = message->tag("badges");
    if (_badges.isValid())
    {
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(c.get());
        if (tc != nullptr)
        {
            auto parsedBadges = parseBadges(_badges.toString());
            tc->setVIP(parsedBadges.contains("vip"));
            tc->setStaff(parsedBadges.contains("staff"));
        }
    }

    // Checking if currentUser is a moderator
    QVariant _mod = message->tag("mod");
    if (_mod.isValid())
    {
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(c.get());
        if (tc != nullptr)
        {
            tc->setMod(_mod == "1");
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

    auto c = getApp()->twitch.server->whispersChannel.get();

    TwitchMessageBuilder builder(c, message, args, message->parameter(1),
                                 false);

    if (builder.isIgnored())
    {
        return;
    }

    builder->flags.set(MessageFlag::Whisper);
    MessagePtr _message = builder.build();
    builder.triggerHighlights();

    getApp()->twitch.server->lastUserThatWhisperedMe.set(builder.userName);

    if (_message->flags.has(MessageFlag::Highlighted))
    {
        getApp()->twitch.server->mentionsChannel->addMessage(_message);
    }

    c->addMessage(_message);

    auto overrideFlags = boost::optional<MessageFlags>(_message->flags);
    overrideFlags->set(MessageFlag::DoNotTriggerNotification);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers)
    {
        getApp()->twitch.server->forEachChannel(
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

        auto b = MessageBuilder(systemMessage, parseTagString(messageText),
                                calculateMessageTimestamp(message));

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

        auto b = MessageBuilder(systemMessage, parseTagString(messageText),
                                calculateMessageTimestamp(message));

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
            formattedMessage, calculateMessageTimestamp(message)));

        return builtMessage;
    }

    // default case
    std::vector<MessagePtr> builtMessages;

    builtMessages.emplace_back(makeSystemMessage(
        message->content(), calculateMessageTimestamp(message)));

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
            getApp()->twitch.server->forEachChannelAndSpecialChannels(
                [msg](const auto &c) {
                    c->addMessage(msg);
                });

            return;
        }

        auto channel = getApp()->twitch.server->getChannelOrEmpty(channelName);

        if (channel->isEmpty())
        {
            qCDebug(chatterinoTwitch)
                << "[IrcManager:handleNoticeMessage] Channel" << channelName
                << "not found in channel manager";
            return;
        }

        QString tags = message->tags().value("msg-id").toString();
        if (tags == "bad_delete_message_error" || tags == "usage_delete")
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/delete <msg-id>\" - can't take more "
                "than one argument"));
        }
        else if (tags == "host_on" || tags == "host_target_went_offline")
        {
            bool hostOn = (tags == "host_on");
            QStringList parts = msg->messageText.split(QLatin1Char(' '));
            if ((hostOn && parts.size() != 3) || (!hostOn && parts.size() != 7))
            {
                return;
            }
            auto &channelName = hostOn ? parts[2] : parts[0];
            if (channelName.size() < 2)
            {
                return;
            }
            if (hostOn)
            {
                channelName.chop(1);
            }
            MessageBuilder builder;
            TwitchMessageBuilder::hostingSystemMessage(channelName, &builder,
                                                       hostOn);
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
    auto channel = getApp()->twitch.server->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

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
    auto channel = getApp()->twitch.server->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

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
