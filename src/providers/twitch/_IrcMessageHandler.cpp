#include "ircmessagehandler.hpp"

#include "application.hpp"
#include "controllers/highlights/highlightcontroller.hpp"
#include "debug/log.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "providers/twitch/twitchhelpers.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/irchelpers.hpp"

#include <IrcMessage>

#include <unordered_set>

using namespace chatterino::singletons;
using namespace chatterino::messages;

namespace chatterino {
namespace providers {
namespace twitch {

IrcMessageHandler &IrcMessageHandler::getInstance()
{
    static IrcMessageHandler instance;
    return instance;
}

void IrcMessageHandler::handlePrivMessage(Communi::IrcPrivateMessage *message, TwitchServer &server)
{
    this->addMessage(message, message->target(), message->content(), server, false,
                     message->isAction());
}

void IrcMessageHandler::addMessage(Communi::IrcMessage *_message, const QString &target,
                                   const QString &content, TwitchServer &server, bool isSub,
                                   bool isAction)
{
    QString channelName;
    if (!trimChannelName(target, channelName)) {
        return;
    }

    auto chan = server.getChannelOrEmpty(channelName);

    if (chan->isEmpty()) {
        return;
    }

    messages::MessageParseArgs args;
    if (isSub) {
        args.trimSubscriberUsername = true;
    }

    if (chan->isBroadcaster()) {
        args.isStaffOrBroadcaster = true;
    }

    TwitchMessageBuilder builder(chan.get(), _message, args, content, isAction);

    if (isSub || !builder.isIgnored()) {
        messages::MessagePtr msg = builder.build();

        if (isSub) {
            msg->flags |= messages::Message::Subscription;
            msg->flags &= ~messages::Message::Highlighted;
        } else {
            if (msg->flags & messages::Message::Highlighted) {
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
    TwitchChannel *twitchChannel = dynamic_cast<twitch::TwitchChannel *>(chan.get());

    if (twitchChannel) {
        // room-id
        decltype(tags.find("xD")) it;

        if ((it = tags.find("room-id")) != tags.end()) {
            auto roomID = it.value().toString();

            twitchChannel->setRoomID(roomID);

            app->resources->loadChannelData(roomID);
        }

        // Room modes
        TwitchChannel::RoomModes roomModes = twitchChannel->getRoomModes();

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
        debug::Log("[IrcMessageHandler:handleClearChatMessage] Twitch channel {} not found",
                   chanName);
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1) {
        chan->disableAllMessages();
        chan->addMessage(Message::createSystemMessage("Chat has been cleared by a moderator."));

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

    auto timeoutMsg = Message::createTimeoutMessage(username, durationInSeconds, reason, false);
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

        twitch::TwitchChannel *tc = dynamic_cast<twitch::TwitchChannel *>(c.get());
        if (tc != nullptr) {
            tc->setMod(_mod == "1");
        }
    }
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    debug::Log("Received whisper!");
    messages::MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto c = app->twitch.server->whispersChannel.get();

    twitch::TwitchMessageBuilder builder(c, message, args, message->parameter(1), false);

    if (!builder.isIgnored()) {
        messages::MessagePtr _message = builder.build();
        _message->flags |= messages::Message::DoNotTriggerNotification;

        if (_message->flags & messages::Message::Highlighted) {
            app->twitch.server->mentionsChannel->addMessage(_message);
        }

        app->twitch.server->lastUserThatWhisperedMe.set(builder.userName);

        c->addMessage(_message);

        if (app->settings->inlineWhispers) {
            app->twitch.server->forEachChannel([_message](ChannelPtr channel) {
                channel->addMessage(_message);  //
            });
        }
    }
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message, TwitchServer &server)
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
        // Sub-specific message. I think it's only allowed for "resub" messages atm
        if (!content.isEmpty()) {
            this->addMessage(message, target, content, server, true, false);
        }
    }

    auto it = tags.find("system-msg");

    if (it != tags.end()) {
        auto newMessage =
            messages::Message::createSystemMessage(util::parseTagString(it.value().toString()));

        newMessage->flags |= messages::Message::Subscription;

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

    auto channel = app->twitch.server->getChannelOrEmpty(message->parameter(0).remove(0, 1));

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
    MessagePtr msg = Message::createSystemMessage(message->content());

    QString channelName;
    if (!trimChannelName(message->target(), channelName)) {
        // Notice wasn't targeted at a single channel, send to all twitch channels
        app->twitch.server->forEachChannelAndSpecialChannels([msg](const auto &c) {
            c->addMessage(msg);  //
        });

        return;
    }

    auto channel = app->twitch.server->getChannelOrEmpty(channelName);

    if (channel->isEmpty()) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager ",
                   channelName);
        return;
    }

    channel->addMessage(msg);
}

void IrcMessageHandler::handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message)
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

        // Display for user who times someone out. This implies you're a moderator, at which point
        // you will be connected to PubSub and receive a better message from there
        "timeout_success",
        "ban_success",
    };

    QVariant v = message->tag("msg-id");
    if (v.isValid()) {
        std::string msgID = v.toString().toStdString();

        if (readConnectionOnlyIDs.find(msgID) != readConnectionOnlyIDs.end()) {
            return;
        }

        debug::Log("Showing notice message from write connection with message id '{}'", msgID);
    }

    this->handleNoticeMessage(message);
}

void IrcMessageHandler::handleJoinMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    auto channel = app->twitch.server->getChannelOrEmpty(message->parameter(0).remove(0, 1));

    if (TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get())) {
        twitchChannel->addJoinedUser(message->nick());
    }
}

void IrcMessageHandler::handlePartMessage(Communi::IrcMessage *message)
{
    auto app = getApp();
    auto channel = app->twitch.server->getChannelOrEmpty(message->parameter(0).remove(0, 1));

    if (TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get())) {
        twitchChannel->addPartedUser(message->nick());
    }
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
