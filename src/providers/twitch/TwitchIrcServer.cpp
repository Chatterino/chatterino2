#include "TwitchIrcServer.hpp"

#include <IrcCommand>
#include <cassert>

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchHelpers.hpp"
#include "util/Helpers.hpp"
#include "util/PostToThread.hpp"

#include <QMetaEnum>

// using namespace Communi;
using namespace std::chrono_literals;

#define TWITCH_PUBSUB_URL "wss://pubsub-edge.twitch.tv"

namespace chatterino {

TwitchIrcServer::TwitchIrcServer()
    : whispersChannel(new Channel("/whispers", Channel::Type::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::Type::TwitchMentions))
    , liveChannel(new Channel("/live", Channel::Type::TwitchLive))
    , watchingChannel(Channel::getEmpty(), Channel::Type::TwitchWatching)
{
    this->initializeIrc();

    this->pubsub = new PubSub(TWITCH_PUBSUB_URL);

    // getSettings()->twitchSeperateWriteConnection.connect([this](auto, auto) {
    // this->connect(); },
    //                                                     this->signalHolder_,
    //                                                     false);
}

void TwitchIrcServer::initialize(Settings &settings, Paths &paths)
{
    getApp()->accounts->twitch.currentUserChanged.connect([this]() {
        postToThread([this] {
            this->connect();
            this->pubsub->setAccount(getApp()->accounts->twitch.getCurrent());
        });
    });

    this->reloadBTTVGlobalEmotes();
    this->reloadFFZGlobalEmotes();
    this->reloadSevenTVGlobalEmotes();

    /* Refresh all twitch channel's live status in bulk every 30 seconds after starting chatterino */
    QObject::connect(&this->bulkLiveStatusTimer_, &QTimer::timeout, [=] {
        this->bulkRefreshLiveStatus();
    });
    this->bulkLiveStatusTimer_.start(30 * 1000);
}

void TwitchIrcServer::initializeConnection(IrcConnection *connection,
                                           ConnectionType type)
{
    std::shared_ptr<TwitchAccount> account =
        getApp()->accounts->twitch.getCurrent();

    qCDebug(chatterinoTwitch) << "logging in as" << account->getUserName();

    // twitch.tv/tags enables IRCv3 tags on messages. See https://dev.twitch.tv/docs/irc/tags
    // twitch.tv/commands enables a bunch of miscellaneous command capabilities. See https://dev.twitch.tv/docs/irc/commands
    // twitch.tv/membership enables the JOIN/PART/NAMES commands. See https://dev.twitch.tv/docs/irc/membership
    // This is enabled so we receive USERSTATE messages when joining channels / typing messages, along with the other command capabilities
    QStringList caps{"twitch.tv/tags", "twitch.tv/commands"};
    if (type != ConnectionType::Write)
    {
        caps.push_back("twitch.tv/membership");
    }

    connection->network()->setSkipCapabilityValidation(true);
    connection->network()->setRequestedCapabilities(caps);

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
    auto channel =
        std::shared_ptr<TwitchChannel>(new TwitchChannel(channelName));
    channel->initialize();

    channel->sendMessageSignal.connect(
        [this, channel = channel.get()](auto &chan, auto &msg, bool &sent) {
            this->onMessageSendRequested(channel, msg, sent);
        });
    channel->sendReplySignal.connect(
        [this, channel = channel.get()](auto &chan, auto &msg, auto &replyId,
                                        bool &sent) {
            this->onReplySendRequested(channel, msg, replyId, sent);
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
    if (command == "JOIN")
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
    else if (command == "RECONNECT")
    {
        this->addGlobalSystemMessage(
            "Twitch Servers requested us to reconnect, reconnecting");
        this->connect();
    }
    else if (command == "GLOBALUSERSTATE")
    {
        handler.handleGlobalUserStateMessage(message);
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
        // Received USERSTATE upon sending PRIVMSG messages
        handler.handleUserStateMessage(message);
    }
    else if (command == "NOTICE")
    {
        // List of expected NOTICE messages on write connection
        // https://git.kotmisia.pl/Mm2PL/docs/src/branch/master/irc_msg_ids.md#command-results
        handler.handleNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }
    else if (command == "RECONNECT")
    {
        this->addGlobalSystemMessage(
            "Twitch Servers requested us to reconnect, reconnecting");
        this->connect();
    }
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

    if (channelName == "/live")
    {
        return this->liveChannel;
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
    func(this->liveChannel);
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

void TwitchIrcServer::bulkRefreshLiveStatus()
{
    auto twitchChans = std::make_shared<QHash<QString, TwitchChannel *>>();

    this->forEachChannel([twitchChans](ChannelPtr chan) {
        auto tc = dynamic_cast<TwitchChannel *>(chan.get());
        if (tc && !tc->roomId().isEmpty())
        {
            twitchChans->insert(tc->roomId(), tc);
        }
    });

    // iterate over batches of channel IDs
    for (const auto &batch : splitListIntoBatches(twitchChans->keys()))
    {
        getHelix()->fetchStreams(
            batch, {},
            [twitchChans](std::vector<HelixStream> streams) {
                for (const auto &stream : streams)
                {
                    // remaining channels will be used later to set their stream status as offline
                    // so we use take(id) to remove it
                    auto tc = twitchChans->take(stream.userId);
                    if (tc == nullptr)
                    {
                        continue;
                    }

                    tc->parseLiveStatus(true, stream);
                }
            },
            []() {
                // failure
            },
            [batch, twitchChans] {
                // All the channels that were not present in fetchStreams response should be assumed to be offline
                // It is necessary to update their stream status in case they've gone live -> offline
                // Otherwise some of them will be marked as live forever
                for (const auto &chID : batch)
                {
                    auto tc = twitchChans->value(chID);
                    // early out in case channel does not exist anymore
                    if (tc == nullptr)
                    {
                        continue;
                    }

                    tc->parseLiveStatus(false, {});
                }
            });
    }
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

bool TwitchIrcServer::prepareToSend(TwitchChannel *channel)
{
    std::lock_guard<std::mutex> guard(this->lastMessageMutex_);

    auto &lastMessage = channel->hasHighRateLimit() ? this->lastMessageMod_
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
                makeSystemMessage("You are sending messages too quickly.");

            channel->addMessage(errorMessage);

            this->lastErrorTimeSpeed_ = now;
        }
        return false;
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
                makeSystemMessage("You are sending too many messages.");

            channel->addMessage(errorMessage);

            this->lastErrorTimeAmount_ = now;
        }
        return false;
    }

    lastMessage.push(now);
    return true;
}

void TwitchIrcServer::onMessageSendRequested(TwitchChannel *channel,
                                             const QString &message, bool &sent)
{
    sent = false;

    bool canSend = this->prepareToSend(channel);
    if (!canSend)
    {
        return;
    }

    this->sendMessage(channel->getName(), message);
    sent = true;
}

void TwitchIrcServer::onReplySendRequested(TwitchChannel *channel,
                                           const QString &message,
                                           const QString &replyId, bool &sent)
{
    sent = false;

    bool canSend = this->prepareToSend(channel);
    if (!canSend)
    {
        return;
    }

    this->sendRawMessage("@reply-parent-msg-id=" + replyId + " PRIVMSG #" +
                         channel->getName() + " :" + message);

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
const SeventvEmotes &TwitchIrcServer::getSeventvEmotes() const
{
    return this->seventv_;
}

void TwitchIrcServer::reloadBTTVGlobalEmotes()
{
    this->bttv.loadEmotes();
}

void TwitchIrcServer::reloadAllBTTVChannelEmotes()
{
    this->forEachChannel([](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->refreshBTTVChannelEmotes(false);
        }
    });
}

void TwitchIrcServer::reloadFFZGlobalEmotes()
{
    this->ffz.loadEmotes();
}

void TwitchIrcServer::reloadAllFFZChannelEmotes()
{
    this->forEachChannel([](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->refreshFFZChannelEmotes(false);
        }
    });
}

void TwitchIrcServer::reloadSevenTVGlobalEmotes()
{
    this->seventv_.loadGlobalEmotes();
}

void TwitchIrcServer::reloadAllSevenTVChannelEmotes()
{
    this->forEachChannel([](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->refreshSevenTVChannelEmotes(false);
        }
    });
}
}  // namespace chatterino
