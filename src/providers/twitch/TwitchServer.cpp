#include "TwitchServer.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "util/PostToThread.hpp"

#include <IrcCommand>
#include <cassert>

// using namespace Communi;
using namespace std::chrono_literals;

namespace chatterino {

TwitchServer::TwitchServer()
    : whispersChannel(new Channel("/whispers", Channel::Type::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::Type::TwitchMentions))
    , watchingChannel(Channel::getEmpty(), Channel::Type::TwitchWatching)
{
    qDebug() << "init TwitchServer";

    this->pubsub = new PubSub;

    // getSettings()->twitchSeperateWriteConnection.connect([this](auto, auto) {
    // this->connect(); },
    //                                                     this->signalHolder_,
    //                                                     false);
}

void TwitchServer::initialize(Settings &settings, Paths &paths)
{
    getApp()->accounts->twitch.currentUserChanged.connect(
        [this]() { postToThread([this] { this->connect(); }); });

    this->twitchBadges.loadTwitchBadges();
    this->bttv.loadEmotes();
    this->ffz.loadEmotes();
}

void TwitchServer::initializeConnection(IrcConnection *connection, bool isRead,
                                        bool isWrite)
{
    this->singleConnection_ = isRead == isWrite;

    std::shared_ptr<TwitchAccount> account =
        getApp()->accounts->twitch.getCurrent();

    qDebug() << "logging in as" << account->getUserName();

    QString username = account->getUserName();
    QString oauthToken = account->getOAuthToken();

    if (!oauthToken.startsWith("oauth:"))
    {
        oauthToken.prepend("oauth:");
    }

    connection->setUserName(username);
    connection->setNickName(username);
    connection->setRealName(username);

    if (!account->isAnon())
    {
        connection->setPassword(oauthToken);
    }

    connection->sendCommand(
        Communi::IrcCommand::createCapability("REQ", "twitch.tv/membership"));
    connection->sendCommand(
        Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    connection->sendCommand(
        Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);
}

std::shared_ptr<Channel> TwitchServer::createChannel(const QString &channelName)
{
    auto channel = std::shared_ptr<TwitchChannel>(new TwitchChannel(
        channelName, this->twitchBadges, this->bttv, this->ffz));
    channel->initialize();

    channel->sendMessageSignal.connect(
        [this, channel = channel.get()](auto &chan, auto &msg, bool &sent) {
            this->onMessageSendRequested(channel, msg, sent);
        });

    return std::shared_ptr<Channel>(channel);
}

void TwitchServer::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
    IrcMessageHandler::getInstance().handlePrivMessage(message, *this);
}

void TwitchServer::messageReceived(Communi::IrcMessage *message)
{
    //    this->readConnection
    if (message->type() == Communi::IrcMessage::Type::Private)
    {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    auto &handler = IrcMessageHandler::getInstance();

    if (command == "ROOMSTATE")
    {
        handler.handleRoomStateMessage(message);
    }
    else if (command == "CLEARCHAT")
    {
        handler.handleClearChatMessage(message);
    }
    else if (command == "USERSTATE")
    {
        handler.handleUserStateMessage(message);
    }
    else if (command == "WHISPER")
    {
        handler.handleWhisperMessage(message);
    }
    else if (command == "USERNOTICE")
    {
        handler.handleUserNoticeMessage(message, *this);
    }
    else if (command == "MODE")
    {
        handler.handleModeMessage(message);
    }
    else if (command == "NOTICE")
    {
        handler.handleNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }
    else if (command == "JOIN")
    {
        handler.handleJoinMessage(message);
    }
    else if (command == "PART")
    {
        handler.handlePartMessage(message);
    }
}

void TwitchServer::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
    switch (message->type())
    {
        case Communi::IrcMessage::Type::Notice:
        {
            IrcMessageHandler::getInstance().handleWriteConnectionNoticeMessage(
                static_cast<Communi::IrcNoticeMessage *>(message));
        }
        break;

        default:;
    }
}

std::shared_ptr<Channel> TwitchServer::getCustomChannel(
    const QString &channelName)
{
    if (channelName == "/whispers")
    {
        return this->whispersChannel;
    }

    if (channelName == "/mentions")
    {
        return this->mentionsChannel;
    }

    return nullptr;
}

void TwitchServer::forEachChannelAndSpecialChannels(
    std::function<void(ChannelPtr)> func)
{
    this->forEachChannel(func);

    func(this->whispersChannel);
    func(this->mentionsChannel);
}

std::shared_ptr<Channel> TwitchServer::getChannelOrEmptyByID(
    const QString &channelId)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (const auto &weakChannel : this->channels)
    {
        auto channel = weakChannel.lock();
        if (!channel)
            continue;

        auto twitchChannel = std::dynamic_pointer_cast<TwitchChannel>(channel);
        if (!twitchChannel)
            continue;

        if (twitchChannel->roomId() == channelId)
        {
            return twitchChannel;
        }
    }

    return Channel::getEmpty();
}

QString TwitchServer::cleanChannelName(const QString &dirtyChannelName)
{
    return dirtyChannelName.toLower();
}

bool TwitchServer::hasSeparateWriteConnection() const
{
    return true;
    // return getSettings()->twitchSeperateWriteConnection;
}

void TwitchServer::onMessageSendRequested(TwitchChannel *channel,
                                          const QString &message, bool &sent)
{
    sent = false;

    {
        std::lock_guard<std::mutex> guard(this->lastMessageMutex_);

        //        std::queue<std::chrono::steady_clock::time_point>
        auto &lastMessage = channel->hasModRights() ? this->lastMessageMod_
                                                    : this->lastMessagePleb_;
        size_t maxMessageCount = channel->hasModRights() ? 99 : 19;
        auto minMessageOffset = (channel->hasModRights() ? 100ms : 1100ms);

        auto now = std::chrono::steady_clock::now();

        // check if you are sending messages too fast
        if (!lastMessage.empty() && lastMessage.back() + minMessageOffset > now)
        {
            if (this->lastErrorTimeSpeed_ + 30s < now)
            {
                auto errorMessage =
                    makeSystemMessage("sending messages too fast");

                channel->addMessage(errorMessage);

                this->lastErrorTimeSpeed_ = now;
            }
            return;
        }

        // remove messages older than 30 seconds
        while (!lastMessage.empty() && lastMessage.front() + 32s < now)
        {
            lastMessage.pop();
        }

        // check if you are sending too many messages
        if (lastMessage.size() >= maxMessageCount)
        {
            if (this->lastErrorTimeAmount_ + 30s < now)
            {
                auto errorMessage =
                    makeSystemMessage("sending too many messages");

                channel->addMessage(errorMessage);

                this->lastErrorTimeAmount_ = now;
            }
            return;
        }

        lastMessage.push(now);
    }

    this->sendMessage(channel->getName(), message);
    sent = true;
}

}  // namespace chatterino
