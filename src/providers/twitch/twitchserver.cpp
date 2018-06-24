#include "twitchserver.hpp"

#include "application.hpp"
#include "controllers/accounts/accountcontroller.hpp"
#include "controllers/highlights/highlightcontroller.hpp"
#include "providers/twitch/ircmessagehandler.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchhelpers.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "util/posttothread.hpp"

#include <IrcCommand>
#include <cassert>

// using namespace Communi;
using namespace chatterino::singletons;

namespace chatterino {
namespace providers {
namespace twitch {

TwitchServer::TwitchServer()
    : whispersChannel(new Channel("/whispers", Channel::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::TwitchMentions))
    , watchingChannel(Channel::getEmpty(), Channel::TwitchWatching)
{
    qDebug() << "init TwitchServer";
}

void TwitchServer::initialize()
{
    getApp()->accounts->twitch.currentUserChanged.connect(
        [this]() { util::postToThread([this] { this->connect(); }); });
}

void TwitchServer::initializeConnection(providers::irc::IrcConnection *connection, bool isRead,
                                        bool isWrite)
{
    std::shared_ptr<TwitchAccount> account = getApp()->accounts->twitch.getCurrent();

    qDebug() << "logging in as" << account->getUserName();

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

    connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/membership"));
    connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);
}

std::shared_ptr<Channel> TwitchServer::createChannel(const QString &channelName)
{
    TwitchChannel *channel = new TwitchChannel(channelName, this->getReadConnection());

    channel->sendMessageSignal.connect([this, channel](auto chan, auto msg, bool &sent) {
        {
            std::lock_guard<std::mutex> guard(this->lastMessageMutex);

            std::queue<QTime> &lastMessage =
                channel->hasModRights() ? this->lastMessageMod : this->lastMessagePleb;
            size_t maxMessageCount = channel->hasModRights() ? 99 : 19;

            QTime now = QTime::currentTime();

            // check if you are sending messages too fast
            if (lastMessage.size() > 0 &&
                lastMessage.back().addMSecs(channel->hasModRights() ? 100 : 1100) > now) {
                if (lastErrorTimeSpeed.addSecs(30) < now) {
                    auto errorMessage =
                        messages::Message::createSystemMessage("sending messages too fast");

                    channel->addMessage(errorMessage);

                    lastErrorTimeSpeed = now;
                }
                return;
            }

            // remove messages older than 30 seconds
            while (lastMessage.size() > 0 && lastMessage.front().addSecs(32) < now) {
                lastMessage.pop();
            }

            // check if you are sending too many messages
            if (lastMessage.size() >= maxMessageCount) {
                if (lastErrorTimeAmount.addSecs(30) < now) {
                    auto errorMessage =
                        messages::Message::createSystemMessage("sending too many messages");

                    channel->addMessage(errorMessage);

                    lastErrorTimeAmount = now;
                }
                return;
            }

            lastMessage.push(now);
        }

        this->sendMessage(chan, msg);
        sent = true;
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
    if (message->type() == Communi::IrcMessage::Type::Private) {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    auto &handler = IrcMessageHandler::getInstance();

    if (command == "ROOMSTATE") {
        handler.handleRoomStateMessage(message);
    } else if (command == "CLEARCHAT") {
        handler.handleClearChatMessage(message);
    } else if (command == "USERSTATE") {
        handler.handleUserStateMessage(message);
    } else if (command == "WHISPER") {
        handler.handleWhisperMessage(message);
    } else if (command == "USERNOTICE") {
        handler.handleUserNoticeMessage(message, *this);
    } else if (command == "MODE") {
        handler.handleModeMessage(message);
    } else if (command == "NOTICE") {
        handler.handleNoticeMessage(static_cast<Communi::IrcNoticeMessage *>(message));
    } else if (command == "JOIN") {
        handler.handleJoinMessage(message);
    } else if (command == "PART") {
        handler.handlePartMessage(message);
    }
}

void TwitchServer::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
    switch (message->type()) {
        case Communi::IrcMessage::Type::Notice: {
            IrcMessageHandler::getInstance().handleWriteConnectionNoticeMessage(
                static_cast<Communi::IrcNoticeMessage *>(message));
        } break;

        default:;
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
    this->forEachChannel(func);

    func(this->whispersChannel);
    func(this->mentionsChannel);
}

std::shared_ptr<Channel> TwitchServer::getChannelOrEmptyByID(const QString &channelID)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (const auto &weakChannel : this->channels) {
        auto channel = weakChannel.lock();
        if (!channel) {
            continue;
        }

        auto twitchChannel = std::dynamic_pointer_cast<TwitchChannel>(channel);
        if (!twitchChannel) {
            continue;
        }

        if (twitchChannel->roomID == channelID) {
            return twitchChannel;
        }
    }

    return Channel::getEmpty();
}

QString TwitchServer::cleanChannelName(const QString &dirtyChannelName)
{
    return dirtyChannelName.toLower();
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
