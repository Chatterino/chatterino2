#include "TwitchIrcServer.hpp"

#include <IrcCommand>
#include <cassert>

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "util/PostToThread.hpp"

// using namespace Communi;
using namespace std::chrono_literals;

namespace chatterino {

TwitchIrcServer::TwitchIrcServer()
    : whispersChannel(new Channel("/whispers", Channel::Type::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::Type::TwitchMentions))
    , watchingChannel(Channel::getEmpty(), Channel::Type::TwitchWatching)
{
    this->initializeIrc();

    this->pubsub = new PubSub;

    // getSettings()->twitchSeperateWriteConnection.connect([this](auto, auto) {
    // this->connect(); },
    //                                                     this->signalHolder_,
    //                                                     false);
}

void TwitchIrcServer::initialize(Settings &settings, Paths &paths)
{
    getApp()->accounts->twitch.currentUserChanged.connect(
        [this]() { postToThread([this] { this->connect(); }); });

    this->twitchBadges.loadTwitchBadges();
    this->bttv.loadEmotes();
    this->ffz.loadEmotes();
}

void TwitchIrcServer::initializeConnection(IrcConnection *connection,
                                           ConnectionType type)
{
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

    // https://dev.twitch.tv/docs/irc/guide/#connecting-to-twitch-irc
    // SSL disabled: irc://irc.chat.twitch.tv:6667 (or port 80)
    // SSL enabled: irc://irc.chat.twitch.tv:6697 (or port 443)
    connection->setHost(Env::get().twitchServerHost);
    connection->setPort(Env::get().twitchServerPort);
    connection->setSecure(Env::get().twitchServerSecure);

    this->open(type);
}

std::shared_ptr<Channel> TwitchIrcServer::createChannel(
    const QString &channelName)
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

void TwitchIrcServer::privateMessageReceived(
    Communi::IrcPrivateMessage *message)
{
    IrcMessageHandler::instance().handlePrivMessage(message, *this);
}

void TwitchIrcServer::readConnectionMessageReceived(
    Communi::IrcMessage *message)
{
    AbstractIrcServer::readConnectionMessageReceived(message);

    if (message->type() == Communi::IrcMessage::Type::Private)
    {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    auto &handler = IrcMessageHandler::instance();

    // Below commands enabled through the twitch.tv/membership CAP REQ
    if (command == "MODE")
    {
        handler.handleModeMessage(message);
    }
    else if (command == "JOIN")
    {
        handler.handleJoinMessage(message);
    }
    else if (command == "PART")
    {
        handler.handlePartMessage(message);
    }
    else if (command == "USERSTATE")
    {
        // Received USERSTATE upon JOINing a channel
        handler.handleUserStateMessage(message);
    }
    else if (command == "ROOMSTATE")
    {
        // Received ROOMSTATE upon JOINing a channel
        handler.handleRoomStateMessage(message);
    }
    else if (command == "CLEARCHAT")
    {
        handler.handleClearChatMessage(message);
    }
    else if (command == "CLEARMSG")
    {
        handler.handleClearMessageMessage(message);
    }
    else if (command == "USERNOTICE")
    {
        handler.handleUserNoticeMessage(message, *this);
    }
    else if (command == "NOTICE")
    {
        handler.handleNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }
    else if (command == "WHISPER")
    {
        handler.handleWhisperMessage(message);
    }
}

void TwitchIrcServer::writeConnectionMessageReceived(
    Communi::IrcMessage *message)
{
    const QString &command = message->command();

    auto &handler = IrcMessageHandler::instance();
    // Below commands enabled through the twitch.tv/commands CAP REQ
    if (command == "USERSTATE")
    {
        // Received USERSTATE upon PRIVMSGing
        handler.handleUserStateMessage(message);
    }
    else if (command == "NOTICE")
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
            // a better message from there.
            "timeout_success",
            "ban_success",

            // Channel suspended notices
            "msg_channel_suspended",
        };

        handler.handleNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }
}

void TwitchIrcServer::onReadConnected(IrcConnection *connection)
{
    // twitch.tv/tags enables IRCv3 tags on messages. See https://dev.twitch.tv/docs/irc/tags/
    // twitch.tv/membership enables the JOIN/PART/MODE/NAMES commands. See https://dev.twitch.tv/docs/irc/membership/
    // twitch.tv/commands enables a bunch of miscellaneous command capabilities. See https://dev.twitch.tv/docs/irc/commands/
    //                    This is enabled here so we receive USERSTATE messages when joining channels
    connection->sendRaw(
        "CAP REQ :twitch.tv/tags twitch.tv/membership twitch.tv/commands");

    AbstractIrcServer::onReadConnected(connection);
}

void TwitchIrcServer::onWriteConnected(IrcConnection *connection)
{
    // twitch.tv/tags enables IRCv3 tags on messages. See https://dev.twitch.tv/docs/irc/tags/
    // twitch.tv/commands enables a bunch of miscellaneous command capabilities. See https://dev.twitch.tv/docs/irc/commands/
    //                    This is enabled here so we receive USERSTATE messages when typing messages, along with the other command capabilities
    connection->sendRaw("CAP REQ :twitch.tv/tags twitch.tv/commands");

    AbstractIrcServer::onWriteConnected(connection);
}

std::shared_ptr<Channel> TwitchIrcServer::getCustomChannel(
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

    if (channelName == "$$$")
    {
        static auto channel =
            std::make_shared<Channel>("$$$", chatterino::Channel::Type::Misc);
        static auto getTimer = [&] {
            for (auto i = 0; i < 1000; i++)
            {
                channel->addMessage(makeSystemMessage(QString::number(i + 1)));
            }

            auto timer = new QTimer;
            QObject::connect(timer, &QTimer::timeout, [] {
                channel->addMessage(
                    makeSystemMessage(QTime::currentTime().toString()));
            });
            timer->start(500);
            return timer;
        }();

        return channel;
    }

    return nullptr;
}

void TwitchIrcServer::forEachChannelAndSpecialChannels(
    std::function<void(ChannelPtr)> func)
{
    this->forEachChannel(func);

    func(this->whispersChannel);
    func(this->mentionsChannel);
}

std::shared_ptr<Channel> TwitchIrcServer::getChannelOrEmptyByID(
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

        if (twitchChannel->roomId() == channelId &&
            twitchChannel->getName().splitRef(":").size() < 3)
        {
            return twitchChannel;
        }
    }

    return Channel::getEmpty();
}

QString TwitchIrcServer::cleanChannelName(const QString &dirtyChannelName)
{
    if (dirtyChannelName.startsWith('#'))
        return dirtyChannelName.mid(1).toLower();
    else
        return dirtyChannelName.toLower();
}

bool TwitchIrcServer::hasSeparateWriteConnection() const
{
    return true;
    // return getSettings()->twitchSeperateWriteConnection;
}

void TwitchIrcServer::onMessageSendRequested(TwitchChannel *channel,
                                             const QString &message, bool &sent)
{
    sent = false;

    {
        std::lock_guard<std::mutex> guard(this->lastMessageMutex_);

        //        std::queue<std::chrono::steady_clock::time_point>
        auto &lastMessage = channel->hasHighRateLimit()
                                ? this->lastMessageMod_
                                : this->lastMessagePleb_;
        size_t maxMessageCount = channel->hasHighRateLimit() ? 99 : 19;
        auto minMessageOffset = (channel->hasHighRateLimit() ? 100ms : 1100ms);

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

const BttvEmotes &TwitchIrcServer::getBttvEmotes() const
{
    return this->bttv;
}
const FfzEmotes &TwitchIrcServer::getFfzEmotes() const
{
    return this->ffz;
}

}  // namespace chatterino
