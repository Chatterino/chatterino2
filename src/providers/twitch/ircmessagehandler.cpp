#include "ircmessagehandler.hpp"

#include <memory>

#include "debug/log.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "providers/twitch/twitchhelpers.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/windowmanager.hpp"

using namespace chatterino::singletons;
using namespace chatterino::messages;

namespace chatterino {
namespace providers {
namespace twitch {

IrcMessageHandler::IrcMessageHandler(singletons::ResourceManager &_resourceManager)
    : resourceManager(_resourceManager)
{
}

IrcMessageHandler &IrcMessageHandler::getInstance()
{
    static IrcMessageHandler instance(singletons::ResourceManager::getInstance());
    return instance;
}

void IrcMessageHandler::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();
    auto iterator = tags.find("room-id");

    if (iterator != tags.end()) {
        auto roomID = iterator.value().toString();

        QStringList words = QString(message->toData()).split("#");

        // ensure the format is valid
        if (words.length() < 2)
            return;

        QString channelName = words.at(1);

        auto channel = TwitchServer::getInstance().getChannel(channelName);

        if (auto twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get())) {
            // set the room id of the channel
            twitchChannel->setRoomID(roomID);
        }

        ResourceManager::getInstance().loadChannelData(roomID);
    }
}

void IrcMessageHandler::handleClearChatMessage(Communi::IrcMessage *message)
{
    // check parameter count
    if (message->parameters().length() < 1) {
        return;
    }

    QString chanName;
    if (!TrimChannelName(message->parameter(0), chanName)) {
        return;
    }

    // get channel
    auto chan = TwitchServer::getInstance().getChannel(chanName);

    if (!chan) {
        debug::Log("[IrcMessageHandler:handleClearChatMessage] Twitch channel {} not found",
                   chanName);
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1) {
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

    // add the notice that the user has been timed out
    LimitedQueueSnapshot<MessagePtr> snapshot = chan->getMessageSnapshot();
    bool addMessage = true;
    int snapshotLength = snapshot.getLength();

    for (int i = std::max(0, snapshotLength - 20); i < snapshotLength; i++) {
        if (snapshot[i]->flags & Message::Timeout && snapshot[i]->loginName == username) {
            MessagePtr replacement(
                Message::createTimeoutMessage(username, durationInSeconds, reason, true));
            chan->replaceMessage(snapshot[i], replacement);
            addMessage = false;
            break;
        }
    }

    if (addMessage) {
        chan->addMessage(Message::createTimeoutMessage(username, durationInSeconds, reason, false));
    }

    // disable the messages from the user
    for (int i = 0; i < snapshotLength; i++) {
        auto &s = snapshot[i];
        if (!(s->flags & Message::Timeout) && s->loginName == username) {
            s->flags.EnableFlag(Message::Disabled);
        }
    }

    // refresh all
    WindowManager::getInstance().repaintVisibleChatWidgets(chan.get());
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    QVariant _mod = message->tag("mod");

    if (_mod.isValid()) {
        QString channelName;
        if (!TrimChannelName(message->parameter(0), channelName)) {
            return;
        }

        auto c = TwitchServer::getInstance().getChannel(channelName);
        twitch::TwitchChannel *tc = dynamic_cast<twitch::TwitchChannel *>(c.get());
        if (tc != nullptr) {
            tc->setMod(_mod == "1");
        }
    }
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *message)
{
    debug::Log("Received whisper!");
    messages::MessageParseArgs args;

    args.isReceivedWhisper = true;

    auto c = TwitchServer::getInstance().whispersChannel.get();

    twitch::TwitchMessageBuilder builder(c, message, message->parameter(1), args);

    if (!builder.isIgnored()) {
        messages::MessagePtr _message = builder.build();
        if (_message->flags & messages::Message::Highlighted) {
            TwitchServer::getInstance().mentionsChannel->addMessage(_message);
        }

        c->addMessage(_message);

        if (SettingManager::getInstance().inlineWhispers) {
            TwitchServer::getInstance().forEachChannel([_message](ChannelPtr channel) {
                channel->addMessage(_message);  //
            });
        }
    }
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcMessageHandler::handleModeMessage(Communi::IrcMessage *message)
{
    auto channel = TwitchServer::getInstance().getChannel(message->parameter(0).remove(0, 1));

    if (message->parameter(1) == "+o") {
        channel->modList.append(message->parameter(2));
    } else if (message->parameter(1) == "-o") {
        channel->modList.append(message->parameter(2));
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    MessagePtr msg = Message::createSystemMessage(message->content());

    QString channelName;
    if (!TrimChannelName(message->target(), channelName)) {
        // Notice wasn't targeted at a single channel, send to all twitch channels
        TwitchServer::getInstance().forEachChannelAndSpecialChannels([msg](const auto &c) {
            c->addMessage(msg);  //
        });

        return;
    }

    auto channel = TwitchServer::getInstance().getChannel(channelName);

    if (!channel) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager",
                   channelName);
        return;
    }

    channel->addMessage(msg);
}

void IrcMessageHandler::handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message)
{
    QVariant v = message->tag("msg-id");
    if (!v.isValid()) {
        return;
    }
    QString msg_id = v.toString();

    static QList<QString> idsToSkip = {"timeout_success", "ban_success"};

    if (idsToSkip.contains(msg_id)) {
        // Already handled in the read-connection
        return;
    }

    this->handleNoticeMessage(message);
}
}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
