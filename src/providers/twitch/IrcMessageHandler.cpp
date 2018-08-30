#include "IrcMessageHandler.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "debug/Log.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IrcHelpers.hpp"

#include <IrcMessage>

#include <unordered_set>

namespace chatterino {

IrcMessageHandler &IrcMessageHandler::getInstance()
{
    static IrcMessageHandler instance;
    return instance;
}

void IrcMessageHandler::handlePrivMessage(Communi::IrcPrivateMessage *message,
                                          TwitchServer &server)
{
    this->addMessage(message, message->target(), message->content(), server,
                     false, message->isAction());
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *_message,
                                   const QString &target,
                                   const QString &content, TwitchServer &server,
                                   bool isSub, bool isAction)
{
    QString channelName;
    if (!trimChannelName(target, channelName)) {
        return;
    }

    auto chan = server.getChannelOrEmpty(channelName);

    if (chan->isEmpty()) {
        return;
    }

    MessageParseArgs args;
    if (isSub) {
        args.trimSubscriberUsername = true;
    }

    if (chan->isBroadcaster()) {
        args.isStaffOrBroadcaster = true;
    }

    TwitchMessageBuilder builder(chan.get(), _message, args, content, isAction);

    if (isSub || !builder.isIgnored()) {
        if (isSub) {
            builder->flags.set(MessageFlag::Subscription);
            builder->flags.unset(MessageFlag::Highlighted);
        }

        auto msg = builder.build();
        auto highlighted = msg->flags.has(MessageFlag::Highlighted);

        if (!isSub) {
            if (highlighted) {
                server.mentionsChannel->addMessage(msg);
                getApp()->highlights->addHighlight(msg);
            }
        }

        chan->addMessage(msg);
    }
}

void IrcMessageHandler::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();
    auto app = getApp();

    // get twitch channel
    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName)) {
        return;
    }
    auto chan = app->twitch.server->getChannelOrEmpty(chanName);

    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(chan.get())) {
        // room-id
        decltype(tags.find("xD")) it;

        if ((it = tags.find("room-id")) != tags.end()) {
            auto roomId = it.value().toString();

            twitchChannel->setRoomId(roomId);
        }

        // Room modes
        {
            auto roomModes = *twitchChannel->accessRoomModes();

            if ((it = tags.find("emote-only")) != tags.end()) {
                roomModes.emoteOnly = it.value() == "1";
            }
            if ((it = tags.find("subs-only")) != tags.end()) {
                roomModes.submode = it.value() == "1";
            }
            if ((it = tags.find("slow")) != tags.end()) {
                roomModes.slowMode = it.value().toInt();
            }
            if ((it = tags.find("r9k")) != tags.end()) {
                roomModes.r9k = it.value() == "1";
            }
            if ((it = tags.find("broadcaster-lang")) != tags.end()) {
                roomModes.broadcasterLang = it.value().toString();
            }
            twitchChannel->setRoomModes(roomModes);
        }

        twitchChannel->roomModesChanged.invoke();
    }
}

void IrcMessageHandler::handleClearChatMessage(Communi::IrcMessage *message)
{
    // check parameter count
    if (message->parameters().length() < 1) {
        return;
    }

    QString chanName;
    if (!trimChannelName(message->parameter(0), chanName)) {
        return;
    }

    auto app = getApp();

    // get channel
    auto chan = app->twitch.server->getChannelOrEmpty(chanName);

    if (chan->isEmpty()) {
        log("[IrcMessageHandler:handleClearChatMessage] Twitch channel {} not "
            "found",
            chanName);
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1) {
        chan->disableAllMessages();
        chan->addMessage(
            makeSystemMessage("Chat has been cleared by a moderator."));

        return;
    }

    // get username, duration and message of the timed out user
    QString username = message->parameter(1);
    QString durationInSeconds, reason;
    QVariant v = message->tag("ban-duration");
    if (v.isValid()) {
        durationInSeconds = v.toString();
    }

    v = message->tag("ban-reason");
    if (v.isValid()) {
        reason = v.toString();
    }

    auto timeoutMsg = MessageBuilder(timeoutMessage, username,
                                     durationInSeconds, reason, false)
                          .release();
    chan->addOrReplaceTimeout(timeoutMsg);

    // refresh all
    app->windows->repaintVisibleChatWidgets(chan.get());
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    QVariant _mod = message->tag("mod");

    if (_mod.isValid()) {
        auto app = getApp();

        QString channelName;
        if (!trimChannelName(message->parameter(0), channelName)) {
            return;
        }

        auto c = app->twitch.server->getChannelOrEmpty(channelName);
        if (c->isEmpty()) {
            return;
        }

        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(c.get());
        if (tc != nullptr) {
            tc->setMod(_mod == "1");
        }
    }
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    log("Received whisper!");
    MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto c = app->twitch.server->whispersChannel.get();

    TwitchMessageBuilder builder(c, message, args, message->parameter(1),
                                 false);

    if (!builder.isIgnored()) {
        app->twitch.server->lastUserThatWhisperedMe.set(builder.userName);

        MessagePtr _message = builder.build();

        if (_message->flags.has(MessageFlag::Highlighted)) {
            app->twitch.server->mentionsChannel->addMessage(_message);
        }

        c->addMessage(_message);

        if (getSettings()->inlineWhispers) {
            app->twitch.server->forEachChannel([_message](ChannelPtr channel) {
                channel->addMessage(_message);  //
            });
        }
    }
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message,
                                                TwitchServer &server)
{
    auto data = message->toData();

    auto tags = message->tags();
    auto parameters = message->parameters();

    auto target = parameters[0];
    QString msgType = tags.value("msg-id", "").toString();
    QString content;
    if (parameters.size() >= 2) {
        content = parameters[1];
    }

    if (msgType == "sub" || msgType == "resub" || msgType == "subgift") {
        // Sub-specific message. I think it's only allowed for "resub" messages
        // atm
        if (!content.isEmpty()) {
            this->addMessage(message, target, content, server, true, false);
        }
    }

    auto it = tags.find("system-msg");

    if (it != tags.end()) {
        auto b = MessageBuilder(systemMessage,
                                parseTagString(it.value().toString()));

        b->flags.set(MessageFlag::Subscription);
        auto newMessage = b.release();

        QString channelName;

        if (message->parameters().size() < 1) {
            return;
        }

        if (!trimChannelName(message->parameter(0), channelName)) {
            return;
        }

        auto chan = server.getChannelOrEmpty(channelName);

        if (!chan->isEmpty()) {
            chan->addMessage(newMessage);
        }
    }
}

void IrcMessageHandler::handleModeMessage(Communi::IrcMessage *message)
{
    auto app = getApp();

    auto channel = app->twitch.server->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

    if (channel->isEmpty()) {
        return;
    }

    if (message->parameter(1) == "+o") {
        channel->modList.append(message->parameter(2));
    } else if (message->parameter(1) == "-o") {
        channel->modList.append(message->parameter(2));
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto app = getApp();
    MessagePtr msg = makeSystemMessage(message->content());

    QString channelName;
    if (!trimChannelName(message->target(), channelName)) {
        // Notice wasn't targeted at a single channel, send to all twitch
        // channels
        app->twitch.server->forEachChannelAndSpecialChannels(
            [msg](const auto &c) {
                c->addMessage(msg);  //
            });

        return;
    }

    auto channel = app->twitch.server->getChannelOrEmpty(channelName);

    if (channel->isEmpty()) {
        log("[IrcManager:handleNoticeMessage] Channel {} not found in channel "
            "manager ",
            channelName);
        return;
    }

    channel->addMessage(msg);
}

void IrcMessageHandler::handleWriteConnectionNoticeMessage(
    Communi::IrcNoticeMessage *message)
{
    static std::unordered_set<std::string> readConnectionOnlyIDs{
        "host_on",
        "host_off",
        "host_target_went_offline",
        "emote_only_on",
        "emote_only_off",
        "slow_on",
        "slow_off",
        "subs_on",
        "subs_off",
        "r9k_on",
        "r9k_off",

        // Display for user who times someone out. This implies you're a
        // moderator, at which point you will be connected to PubSub and receive
        // a better message from there
        "timeout_success",
        "ban_success",
    };

    QVariant v = message->tag("msg-id");
    if (v.isValid()) {
        std::string msgID = v.toString().toStdString();

        if (readConnectionOnlyIDs.find(msgID) != readConnectionOnlyIDs.end()) {
            return;
        }

        log("Showing notice message from write connection with message id '{}'",
            msgID);
    }

    this->handleNoticeMessage(message);
}

void IrcMessageHandler::handleJoinMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    auto channel = app->twitch.server->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

    if (TwitchChannel *twitchChannel =
            dynamic_cast<TwitchChannel *>(channel.get())) {
        twitchChannel->addJoinedUser(message->nick());
    }
}

void IrcMessageHandler::handlePartMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    auto channel = app->twitch.server->getChannelOrEmpty(
        message->parameter(0).remove(0, 1));

    if (TwitchChannel *twitchChannel =
            dynamic_cast<TwitchChannel *>(channel.get())) {
        twitchChannel->addPartedUser(message->nick());
    }
}

}  // namespace chatterino
