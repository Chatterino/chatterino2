#include "twitchserver.hpp"

#include "providers/twitch/ircmessagehandler.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "singletons/accountmanager.hpp"
#include "util/posttothread.hpp"

#include <cassert>

using namespace Communi;
using namespace chatterino::singletons;

namespace chatterino {
namespace providers {
namespace twitch {
TwitchServer::TwitchServer()
    : whispersChannel(new Channel("/mentions"))
    , mentionsChannel(new Channel("/mentions"))
{
    AccountManager::getInstance().Twitch.userChanged.connect([this]() {  //
        util::postToThread([this] { this->connect(); });
    });
}

TwitchServer &TwitchServer::getInstance()
{
    static TwitchServer s;
    return s;
}

void TwitchServer::initializeConnection(IrcConnection *connection, bool isRead, bool isWrite)
{
    std::shared_ptr<TwitchAccount> account = AccountManager::getInstance().Twitch.getCurrent();

    QString username = account->getUserName();
    //    QString oauthClient = account->getOAuthClient();
    QString oauthToken = account->getOAuthToken();

    if (!oauthToken.startsWith("oauth:")) {
        oauthToken.prepend("oauth:");
    }

    connection->setUserName(username);
    connection->setNickName(username);
    connection->setRealName(username);

    if (!account->isAnon()) {
        connection->setPassword(oauthToken);

        // fourtf: ignored users
        //        this->refreshIgnoredUsers(username, oauthClient, oauthToken);
    }

    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/membership"));
    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);
}

std::shared_ptr<Channel> TwitchServer::createChannel(const QString &channelName)
{
    TwitchChannel *channel = new TwitchChannel(channelName, this->getReadConnection());

    channel->sendMessageSignal.connect(
        [this](auto chan, auto msg) { this->sendMessage(chan, msg); });

    return std::shared_ptr<Channel>(channel);
}

void TwitchServer::privateMessageReceived(IrcPrivateMessage *message)
{
    this->onPrivateMessage.invoke(message);
    auto chan = TwitchServer::getInstance().getChannel(message->target().mid(1));

    if (!chan) {
        return;
    }

    messages::MessageParseArgs args;

    TwitchMessageBuilder builder(chan.get(), message, args);

    if (!builder.isIgnored() && message->nick() != "airbrushgrenade") {
        messages::MessagePtr _message = builder.build();
        if (_message->flags & messages::Message::Highlighted) {
            TwitchServer::getInstance().mentionsChannel->addMessage(_message);
        }

        chan->addMessage(_message);
    }
}

void TwitchServer::messageReceived(IrcMessage *message)
{
    //    this->readConnection
    if (message->type() == IrcMessage::Type::Private) {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    if (command == "ROOMSTATE") {
        IrcMessageHandler::getInstance().handleRoomStateMessage(message);
    } else if (command == "CLEARCHAT") {
        IrcMessageHandler::getInstance().handleClearChatMessage(message);
    } else if (command == "USERSTATE") {
        IrcMessageHandler::getInstance().handleUserStateMessage(message);
    } else if (command == "WHISPER") {
        IrcMessageHandler::getInstance().handleWhisperMessage(message);
    } else if (command == "USERNOTICE") {
        IrcMessageHandler::getInstance().handleUserNoticeMessage(message);
    } else if (command == "MODE") {
        IrcMessageHandler::getInstance().handleModeMessage(message);
    } else if (command == "NOTICE") {
        IrcMessageHandler::getInstance().handleNoticeMessage(
            static_cast<IrcNoticeMessage *>(message));
    }
}

void TwitchServer::writeConnectionMessageReceived(IrcMessage *message)
{
    switch (message->type()) {
        case IrcMessage::Type::Notice: {
            IrcMessageHandler::getInstance().handleWriteConnectionNoticeMessage(
                static_cast<IrcNoticeMessage *>(message));
        } break;
    }
}

std::shared_ptr<Channel> TwitchServer::getCustomChannel(const QString &channelName)
{
    if (channelName == "/whispers") {
        return whispersChannel;
    }

    if (channelName == "/mentions") {
        return mentionsChannel;
    }

    return nullptr;
}

void TwitchServer::forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (std::weak_ptr<Channel> &weak : this->channels) {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan) {
            continue;
        }

        func(chan);
    }

    func(this->whispersChannel);
    func(this->mentionsChannel);
}
}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
