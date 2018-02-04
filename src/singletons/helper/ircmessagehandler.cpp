#include "ircmessagehandler.hpp"

#include <memory>

#include "debug/log.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/windowmanager.hpp"
#include "twitch/twitchchannel.hpp"
#include "twitch/twitchmessagebuilder.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {
namespace helper {

IrcMessageHandler::IrcMessageHandler(ChannelManager &_channelManager,
                                     ResourceManager &_resourceManager)
    : channelManager(_channelManager)
    , resourceManager(_resourceManager)
{
}

IrcMessageHandler &IrcMessageHandler::getInstance()
{
    static IrcMessageHandler instance(ChannelManager::getInstance(),
                                      ResourceManager::getInstance());
    return instance;
}

void IrcMessageHandler::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();

    auto iterator = tags.find("room-id");

    if (iterator != tags.end()) {
        auto roomID = iterator.value().toString();

        auto channel =
            this->channelManager.getTwitchChannel(QString(message->toData()).split("#").at(1));
        auto twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get());
        if (twitchChannel != nullptr) {
            twitchChannel->setRoomID(roomID);
        }

        this->resourceManager.loadChannelData(roomID);
    }
}

void IrcMessageHandler::handleClearChatMessage(Communi::IrcMessage *message)
{
    assert(message->parameters().length() >= 1);

    auto rawChannelName = message->parameter(0);

    assert(rawChannelName.length() >= 2);

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log(
            "[IrcMessageHandler:handleClearChatMessage] Channel {} not found in channel manager",
            trimmedChannelName);
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1) {
        c->addMessage(Message::createSystemMessage("Chat has been cleared by a moderator."));

        return;
    }

    assert(message->parameters().length() >= 2);

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
    LimitedQueueSnapshot<MessagePtr> snapshot = c->getMessageSnapshot();
    bool addMessage = true;
    int snapshotLength = snapshot.getLength();

    for (int i = std::max(0, snapshotLength - 20); i < snapshotLength; i++) {
        if (snapshot[i]->flags & Message::Timeout && snapshot[i]->loginName == username) {
            MessagePtr replacement(
                Message::createTimeoutMessage(username, durationInSeconds, reason, true));
            c->replaceMessage(snapshot[i], replacement);
            addMessage = false;
            break;
        }
    }

    if (addMessage) {
        c->addMessage(Message::createTimeoutMessage(username, durationInSeconds, reason, false));
    }

    // disable the messages from the user
    for (int i = 0; i < snapshotLength; i++) {
        if (!(snapshot[i]->flags & Message::Timeout) && snapshot[i]->loginName == username) {
            snapshot[i]->flags &= Message::Disabled;
        }
    }

    // refresh all
    WindowManager::getInstance().repaintVisibleChatWidgets(c.get());
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    QVariant _mod = message->tag("mod");

    if (_mod.isValid()) {
        auto rawChannelName = message->parameters().at(0);
        auto trimmedChannelName = rawChannelName.mid(1);

        auto c = this->channelManager.getTwitchChannel(trimmedChannelName);
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

    auto c = this->channelManager.whispersChannel.get();

    twitch::TwitchMessageBuilder builder(c, message, message->parameter(1), args);

    if (!builder.isIgnored()) {
        messages::MessagePtr _message = builder.build();
        if (_message->flags & messages::Message::Highlighted) {
            singletons::ChannelManager::getInstance().mentionsChannel->addMessage(_message);
        }

        c->addMessage(_message);

        if (SettingManager::getInstance().inlineWhispers) {
            this->channelManager.doOnAllNormalChannels([_message](ChannelPtr channel) {
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
    auto channel = channelManager.getTwitchChannel(message->parameter(0).remove(0, 1));
    if (message->parameter(1) == "+o") {
        channel->modList.append(message->parameter(2));
    } else if (message->parameter(1) == "-o") {
        channel->modList.append(message->parameter(2));
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto rawChannelName = message->target();

    bool broadcast = rawChannelName.length() < 2;
    MessagePtr msg = Message::createSystemMessage(message->content());

    if (broadcast) {
        this->channelManager.doOnAll([msg](const auto &c) {
            c->addMessage(msg);  //
        });

        return;
    }

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager",
                   trimmedChannelName);
        return;
    }

    c->addMessage(msg);
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

}  // namespace helper
}  // namespace singletons
}  // namespace chatterino
