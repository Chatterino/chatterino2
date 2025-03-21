#include "providers/twitch/TwitchIrcServer.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/irc/IrcConnection2.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/seventv/SeventvEventAPI.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/pubsubmessages/AutoMod.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"
#include "util/RatelimitBucket.hpp"
#include "util/Twitch.hpp"

#include <IrcCommand>
#include <IrcMessage>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QCoreApplication>
#include <QMetaEnum>

#include <cassert>
#include <functional>
#include <mutex>

using namespace std::chrono_literals;

namespace {

// Ratelimits for joinBucket_
constexpr int JOIN_RATELIMIT_BUDGET = 18;
constexpr int JOIN_RATELIMIT_COOLDOWN = 12500;

using namespace chatterino;

void sendHelixMessage(const std::shared_ptr<TwitchChannel> &channel,
                      const QString &message, const QString &replyParentId = {})
{
    auto broadcasterID = channel->roomId();
    if (broadcasterID.isEmpty())
    {
        channel->addSystemMessage(
            "Sending messages in this channel isn't possible.");
        return;
    }

    getHelix()->sendChatMessage(
        {
            .broadcasterID = broadcasterID,
            .senderID =
                getApp()->getAccounts()->twitch.getCurrent()->getUserId(),
            .message = message,
            .replyParentMessageID = replyParentId,
        },
        [weak = std::weak_ptr(channel)](const auto &res) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (res.isSent)
            {
                return;
            }

            if (res.dropReason)
            {
                chan->addSystemMessage(res.dropReason->message);
            }
            else
            {
                chan->addSystemMessage("Your message was not sent.");
            }
        },
        [weak = std::weak_ptr(channel)](auto error, auto message) {
            auto chan = weak.lock();
            if (!chan)
            {
                return;
            }

            if (message.isEmpty())
            {
                message = "(empty message)";
            }

            using Error = decltype(error);

            auto errorMessage = [&]() -> QString {
                switch (error)
                {
                    case Error::MissingText:
                        return "You can't send an empty message.";
                    case Error::BadRequest:
                        return "Failed to send message: " + message;
                    case Error::Forbidden:
                        return "You are not allowed to send messages in this "
                               "channel.";
                    case Error::MessageTooLarge:
                        return "Your message was too long.";
                    case Error::UserMissingScope:
                        return "Missing required scope. Re-login with your "
                               "account and try again.";
                    case Error::Forwarded:
                        return message;
                    case Error::Unknown:
                    default:
                        return "Unknown error: " + message;
                }
            }();
            chan->addSystemMessage(errorMessage);
        });
}

/// Returns true if chat messages should be sent over Helix
bool shouldSendHelixChat()
{
    switch (getSettings()->chatSendProtocol)
    {
        case ChatSendProtocol::Helix:
            return true;
        case ChatSendProtocol::Default:
        case ChatSendProtocol::IRC:
            return false;
        default:
            assert(false && "Invalid chat protocol value");
            return false;
    }
}

}  // namespace

namespace chatterino {

using namespace literals;

TwitchIrcServer::TwitchIrcServer()
    : whispersChannel(new Channel("/whispers", Channel::Type::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::Type::TwitchMentions))
    , liveChannel(new Channel("/live", Channel::Type::TwitchLive))
    , automodChannel(new Channel("/automod", Channel::Type::TwitchAutomod))
    , watchingChannel(Channel::getEmpty(), Channel::Type::TwitchWatching)
{
    // Initialize the connections
    // XXX: don't create write connection if there is no separate write connection.
    this->writeConnection_.reset(new IrcConnection);
    this->writeConnection_->moveToThread(
        QCoreApplication::instance()->thread());

    // Apply a leaky bucket rate limiting to JOIN messages
    auto actuallyJoin = [&](QString message) {
        if (!this->channels.contains(message))
        {
            return;
        }
        this->readConnection_->sendRaw("JOIN #" + message);
    };
    this->joinBucket_.reset(new RatelimitBucket(
        JOIN_RATELIMIT_BUDGET, JOIN_RATELIMIT_COOLDOWN, actuallyJoin, this));

    QObject::connect(this->writeConnection_.get(),
                     &Communi::IrcConnection::messageReceived, this,
                     [this](auto msg) {
                         this->writeConnectionMessageReceived(msg);
                     });
    QObject::connect(this->writeConnection_.get(),
                     &Communi::IrcConnection::connected, this, [this] {
                         this->onWriteConnected(this->writeConnection_.get());
                     });
    this->connections_.managedConnect(
        this->writeConnection_->connectionLost, [this](bool timeout) {
            qCDebug(chatterinoIrc)
                << "Write connection reconnect requested. Timeout:" << timeout;
            this->writeConnection_->smartReconnect();
        });

    // Listen to read connection message signals
    this->readConnection_.reset(new IrcConnection);
    this->readConnection_->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->readConnection_.get(),
                     &Communi::IrcConnection::messageReceived, this,
                     [this](auto msg) {
                         this->readConnectionMessageReceived(msg);
                     });
    QObject::connect(this->readConnection_.get(),
                     &Communi::IrcConnection::privateMessageReceived, this,
                     [this](auto msg) {
                         this->privateMessageReceived(msg);
                     });
    QObject::connect(this->readConnection_.get(),
                     &Communi::IrcConnection::connected, this, [this] {
                         this->onReadConnected(this->readConnection_.get());
                     });
    QObject::connect(this->readConnection_.get(),
                     &Communi::IrcConnection::disconnected, this, [this] {
                         this->onDisconnected();
                     });
    this->connections_.managedConnect(
        this->readConnection_->connectionLost, [this](bool timeout) {
            qCDebug(chatterinoIrc)
                << "Read connection reconnect requested. Timeout:" << timeout;
            if (timeout)
            {
                // Show additional message since this is going to interrupt a
                // connection that is still "connected"
                this->addGlobalSystemMessage(
                    "Server connection timed out, reconnecting");
            }
            this->readConnection_->smartReconnect();
        });
    this->connections_.managedConnect(this->readConnection_->heartbeat, [this] {
        this->markChannelsConnected();
    });
}

void TwitchIrcServer::initialize()
{
    getApp()->getAccounts()->twitch.currentUserChanged.connect([this]() {
        postToThread([this] {
            this->connect();
        });
    });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.chatCleared,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, actor{action.source.login}] {
                auto now = QDateTime::currentDateTime();
                chan->addOrReplaceClearChat(
                    MessageBuilder::makeClearChatMessage(now, actor), now);
                if (getSettings()->hideModerated)
                {
                    // XXX: This is expensive. We could use a layout request if the layout
                    //      would store the previous message flags.
                    getApp()->getWindows()->forceLayoutChannelViews();
                }
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.modeChanged,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text =
                QString("%1 turned %2 %3 mode.")
                    .arg(action.source.login)
                    .arg(action.state == ModeChangedAction::State::On ? "on"
                                                                      : "off")
                    .arg(action.getModeName());

            if (action.duration > 0)
            {
                text += QString(" (%1 seconds)").arg(action.duration);
            }

            postToThread([chan, text] {
                chan->addSystemMessage(text);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.moderationStateChanged,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text;

            text = QString("%1 %2 %3.")
                       .arg(action.source.login,
                            (action.modded ? "modded" : "unmodded"),
                            action.target.login);

            postToThread([chan, text] {
                chan->addSystemMessage(text);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.userBanned,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                // TODO: Can we utilize some pubsub time field? maybe not worth
                auto time = QDateTime::currentDateTime();
                MessageBuilder msg(action, time);
                msg->flags.set(MessageFlag::PubSub);
                chan->addOrReplaceTimeout(msg.release(),
                                          QDateTime::currentDateTime());
                if (getSettings()->hideModerated)
                {
                    // XXX: This is expensive. We could use a layout request if the layout
                    //      would store the previous message flags.
                    getApp()->getWindows()->forceLayoutChannelViews();
                }
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.userWarned,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            // TODO: Resolve the moderator's user ID into a full user here, so message can look better
            postToThread([chan, action] {
                MessageBuilder msg(action);
                msg->flags.set(MessageFlag::PubSub);
                chan->addMessage(msg.release(), MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.messageDeleted,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty() || getSettings()->hideDeletionActions)
            {
                return;
            }

            auto msg = MessageBuilder::makeDeletionMessageFromPubSub(action);

            postToThread([chan, msg] {
                auto replaced = false;
                LimitedQueueSnapshot<MessagePtr> snapshot =
                    chan->getMessageSnapshot();
                int snapshotLength = snapshot.size();

                // without parens it doesn't build on windows
                int end = (std::max)(0, snapshotLength - 200);

                for (int i = snapshotLength - 1; i >= end; --i)
                {
                    const auto &s = snapshot[i];
                    if (!s->flags.has(MessageFlag::PubSub) &&
                        s->timeoutUser == msg->timeoutUser)
                    {
                        chan->replaceMessage(s, msg);
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                {
                    chan->addMessage(msg, MessageContext::Original);
                }
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.userUnbanned,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            // TODO: Can we utilize some pubsub time field? maybe not worth
            auto time = QDateTime::currentDateTime();
            auto msg = MessageBuilder(action, time).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.suspiciousMessageReceived,
        [this](const auto &action) {
            if (action.treatment ==
                PubSubLowTrustUsersMessage::Treatment::INVALID)
            {
                qCWarning(chatterinoTwitch)
                    << "Received suspicious message with unknown "
                       "treatment:"
                    << action.treatmentString;
                return;
            }

            // monitored chats are received over irc; in the future, we will use pubsub instead
            if (action.treatment !=
                PubSubLowTrustUsersMessage::Treatment::Restricted)
            {
                return;
            }

            if (getApp()->getStreamerMode()->shouldHideModActions())
            {
                // NOTE: This completely stops the building of this action, rathern than only hiding it.
                // If the user disabled streamer mode or the setting, there will be messages missing
                return;
            }

            auto chan = this->getChannelOrEmptyByID(action.channelID);

            if (chan->isEmpty())
            {
                return;
            }

            auto twitchChannel = std::dynamic_pointer_cast<TwitchChannel>(chan);
            if (!twitchChannel)
            {
                return;
            }

            postToThread([twitchChannel, action] {
                const auto p = MessageBuilder::makeLowTrustUserMessage(
                    action, twitchChannel->getName(), twitchChannel.get());
                twitchChannel->addMessage(p.first, MessageContext::Original);
                twitchChannel->addMessage(p.second, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.suspiciousTreatmentUpdated,
        [this](const auto &action) {
            if (action.treatment ==
                PubSubLowTrustUsersMessage::Treatment::INVALID)
            {
                qCWarning(chatterinoTwitch)
                    << "Received suspicious user update with unknown "
                       "treatment:"
                    << action.treatmentString;
                return;
            }

            if (action.updatedByUserLogin.isEmpty())
            {
                return;
            }

            if (getApp()->getStreamerMode()->shouldHideModActions())
            {
                // NOTE: This completely stops the building of this action, rathern than only hiding it.
                // If the user disabled streamer mode or the setting, there will be messages missing
                return;
            }

            auto chan = this->getChannelOrEmptyByID(action.channelID);
            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                auto msg = MessageBuilder::makeLowTrustUpdateMessage(action);
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.autoModMessageCaught,
        [this](const auto &msg, const QString &channelID) {
            auto chan = this->getChannelOrEmptyByID(channelID);
            if (chan->isEmpty())
            {
                return;
            }

            switch (msg.type)
            {
                case PubSubAutoModQueueMessage::Type::AutoModCaughtMessage: {
                    if (msg.status == "PENDING")
                    {
                        AutomodAction action(msg.data, channelID);
                        if (msg.reason ==
                            PubSubAutoModQueueMessage::Reason::BlockedTerm)
                        {
                            auto numBlockedTermsMatched =
                                msg.blockedTermsFound.size();
                            auto hideBlockedTerms =
                                getSettings()
                                    ->streamerModeHideBlockedTermText &&
                                getApp()->getStreamerMode()->isEnabled();
                            if (!msg.blockedTermsFound.empty())
                            {
                                if (hideBlockedTerms)
                                {
                                    action.reason =
                                        u"matches %1 blocked term%2"_s
                                            .arg(numBlockedTermsMatched)
                                            .arg(numBlockedTermsMatched > 1
                                                     ? u"s"
                                                     : u"");
                                }
                                else
                                {
                                    QStringList blockedTerms(
                                        msg.blockedTermsFound.begin(),
                                        msg.blockedTermsFound.end());
                                    action.reason =
                                        u"matches %1 blocked term%2 \"%3\""_s
                                            .arg(numBlockedTermsMatched)
                                            .arg(numBlockedTermsMatched > 1
                                                     ? u"s"
                                                     : u"")
                                            .arg(blockedTerms.join(u"\", \""));
                                }
                            }
                            else
                            {
                                action.reason = "blocked term usage";
                            }
                        }
                        else
                        {
                            action.reason = QString("%1 level %2")
                                                .arg(msg.contentCategory)
                                                .arg(msg.contentLevel);
                        }

                        action.msgID = msg.messageID;
                        action.message = msg.messageText;
                        action.reasonCode = msg.reason;

                        // this message also contains per-word automod data, which could be implemented

                        // extract sender data manually because Twitch loves not being consistent
                        QString senderDisplayName =
                            msg.senderUserDisplayName;  // Might be transformed later
                        bool hasLocalizedName = false;
                        if (!msg.senderUserDisplayName.isEmpty())
                        {
                            // check for non-ascii display names
                            if (QString::compare(msg.senderUserDisplayName,
                                                 msg.senderUserLogin,
                                                 Qt::CaseInsensitive) != 0)
                            {
                                hasLocalizedName = true;
                            }
                        }
                        QColor senderColor = msg.senderUserChatColor;
                        QString senderColor_;
                        if (!senderColor.isValid() &&
                            getSettings()->colorizeNicknames)
                        {
                            // color may be not present if user is a grey-name
                            senderColor = getRandomColor(msg.senderUserID);
                        }

                        // handle username style based on prefered setting
                        switch (getSettings()->usernameDisplayMode.getValue())
                        {
                            case UsernameDisplayMode::Username: {
                                if (hasLocalizedName)
                                {
                                    senderDisplayName = msg.senderUserLogin;
                                }
                                break;
                            }
                            case UsernameDisplayMode::LocalizedName: {
                                break;
                            }
                            case UsernameDisplayMode::
                                UsernameAndLocalizedName: {
                                if (hasLocalizedName)
                                {
                                    senderDisplayName = QString("%1(%2)").arg(
                                        msg.senderUserLogin,
                                        msg.senderUserDisplayName);
                                }
                                break;
                            }
                        }

                        action.target =
                            ActionUser{msg.senderUserID, msg.senderUserLogin,
                                       senderDisplayName, senderColor};
                        postToThread([chan, action] {
                            const auto p = MessageBuilder::makeAutomodMessage(
                                action, chan->getName());
                            chan->addMessage(p.first, MessageContext::Original);
                            chan->addMessage(p.second,
                                             MessageContext::Original);

                            getApp()
                                ->getTwitch()
                                ->getAutomodChannel()
                                ->addMessage(p.first, MessageContext::Original);
                            getApp()
                                ->getTwitch()
                                ->getAutomodChannel()
                                ->addMessage(p.second,
                                             MessageContext::Original);

                            if (getSettings()->showAutomodInMentions)
                            {
                                getApp()
                                    ->getTwitch()
                                    ->getMentionsChannel()
                                    ->addMessage(p.first,
                                                 MessageContext::Original);
                                getApp()
                                    ->getTwitch()
                                    ->getMentionsChannel()
                                    ->addMessage(p.second,
                                                 MessageContext::Original);
                            }
                        });
                    }
                    else
                    {
                        // Gray out approve/deny button upon "ALLOWED" and "DENIED" statuses
                        // They are versions of automod_message_(denied|approved) but for mods.
                        chan->disableMessage("automod_" + msg.messageID);
                    }
                }
                break;

                case PubSubAutoModQueueMessage::Type::INVALID:
                default: {
                }
                break;
            }
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.autoModMessageBlocked,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                const auto p =
                    MessageBuilder::makeAutomodMessage(action, chan->getName());
                chan->addMessage(p.first, MessageContext::Original);
                chan->addMessage(p.second, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.automodUserMessage,
        [this](const auto &action) {
            if (getApp()->getStreamerMode()->shouldHideModActions())
            {
                // NOTE: This completely stops the building of this action, rathern than only hiding it.
                // If the user disabled streamer mode or the setting, there will be messages missing
                return;
            }
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.automodInfoMessage,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                const auto p = MessageBuilder::makeAutomodInfoMessage(action);
                chan->addMessage(p, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.raidStarted,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->moderation.raidCanceled,
        [this](const auto &action) {
            auto chan = this->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    this->connections_.managedConnect(
        getApp()->getTwitchPubSub()->pointReward.redeemed, [this](auto &data) {
            QString channelId = data.value("channel_id").toString();
            if (channelId.isEmpty())
            {
                qCDebug(chatterinoApp)
                    << "Couldn't find channel id of point reward";
                return;
            }

            auto chan = this->getChannelOrEmptyByID(channelId);

            auto reward = ChannelPointReward(data);

            postToThread([chan, reward] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addChannelPointReward(reward);
                }
            });
        });
}

void TwitchIrcServer::initializeConnection(IrcConnection *connection,
                                           ConnectionType type)
{
    std::shared_ptr<TwitchAccount> account =
        getApp()->getAccounts()->twitch.getCurrent();

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

    // https://dev.twitch.tv/docs/irc#connecting-to-the-twitch-irc-server
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
    auto channel = std::make_shared<TwitchChannel>(channelName);
    channel->initialize();

    // We can safely ignore these signal connections since the TwitchIrcServer is only
    // ever destroyed when the full Application state is about to be destroyed, at which point
    // no Channel's should live
    // NOTE: CHANNEL_LIFETIME
    std::ignore = channel->sendMessageSignal.connect(
        [this, channel = std::weak_ptr(channel)](auto &chan, auto &msg,
                                                 bool &sent) {
            auto c = channel.lock();
            if (!c)
            {
                return;
            }
            this->onMessageSendRequested(c, msg, sent);
        });
    std::ignore = channel->sendReplySignal.connect(
        [this, channel = std::weak_ptr(channel)](auto &chan, auto &msg,
                                                 auto &replyId, bool &sent) {
            auto c = channel.lock();
            if (!c)
            {
                return;
            }
            this->onReplySendRequested(c, msg, replyId, sent);
        });

    return channel;
}

void TwitchIrcServer::privateMessageReceived(
    Communi::IrcPrivateMessage *message)
{
    IrcMessageHandler::instance().handlePrivMessage(message, *this);
}

void TwitchIrcServer::readConnectionMessageReceived(
    Communi::IrcMessage *message)
{
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
        this->markChannelsConnected();
        this->connect();
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

void TwitchIrcServer::onReadConnected(IrcConnection *connection)
{
    (void)connection;

    std::vector<ChannelPtr> activeChannels;
    {
        std::lock_guard lock(this->channelMutex);

        activeChannels.reserve(this->channels.size());
        for (const auto &weak : this->channels)
        {
            if (auto channel = weak.lock())
            {
                activeChannels.push_back(channel);
            }
        }
    }

    // put the visible channels first
    auto visible = getApp()->getWindows()->getVisibleChannelNames();

    std::ranges::stable_partition(activeChannels, [&](const auto &chan) {
        return visible.contains(chan->getName());
    });

    // join channels
    for (const auto &channel : activeChannels)
    {
        this->joinBucket_->send(channel->getName());
    }

    // connected/disconnected message
    auto connectedMsg = makeSystemMessage("connected");
    connectedMsg->flags.set(MessageFlag::ConnectedMessage);
    auto reconnected = makeSystemMessage("reconnected");
    reconnected->flags.set(MessageFlag::ConnectedMessage);

    for (const auto &chan : activeChannels)
    {
        LimitedQueueSnapshot<MessagePtr> snapshot = chan->getMessageSnapshot();

        bool replaceMessage =
            snapshot.size() > 0 && snapshot[snapshot.size() - 1]->flags.has(
                                       MessageFlag::DisconnectedMessage);

        if (replaceMessage)
        {
            chan->replaceMessage(snapshot[snapshot.size() - 1], reconnected);
        }
        else
        {
            chan->addMessage(connectedMsg, MessageContext::Original);
        }
    }

    this->falloffCounter_ = 1;
}

void TwitchIrcServer::onWriteConnected(IrcConnection *connection)
{
    (void)connection;
}

void TwitchIrcServer::onDisconnected()
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    MessageBuilder b(systemMessage, "disconnected");
    b->flags.set(MessageFlag::DisconnectedMessage);
    auto disconnectedMsg = b.release();

    for (std::weak_ptr<Channel> &weak : this->channels.values())
    {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan)
        {
            continue;
        }

        chan->addMessage(disconnectedMsg, MessageContext::Original);

        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->markDisconnected();
        }
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

    if (channelName == "/automod")
    {
        return this->automodChannel;
    }

    static auto getTimer = [](ChannelPtr channel, int msBetweenMessages,
                              bool addInitialMessages) {
        if (addInitialMessages)
        {
            for (auto i = 0; i < 1000; i++)
            {
                channel->addSystemMessage(QString::number(i + 1));
            }
        }

        auto *timer = new QTimer;
        QObject::connect(timer, &QTimer::timeout, [channel] {
            channel->addSystemMessage(QTime::currentTime().toString());
        });
        timer->start(msBetweenMessages);
        return timer;
    };

    if (channelName == "$$$")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 500, true);

        return channel;
    }
    if (channelName == "$$$:e")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 500, false);

        return channel;
    }
    if (channelName == "$$$$")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 250, true);

        return channel;
    }
    if (channelName == "$$$$:e")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 250, false);

        return channel;
    }
    if (channelName == "$$$$$")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 100, true);

        return channel;
    }
    if (channelName == "$$$$$:e")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 100, false);

        return channel;
    }
    if (channelName == "$$$$$$")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 50, true);

        return channel;
    }
    if (channelName == "$$$$$$:e")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 50, false);

        return channel;
    }
    if (channelName == "$$$$$$$")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 25, true);

        return channel;
    }
    if (channelName == "$$$$$$$:e")
    {
        static auto channel = std::make_shared<Channel>(
            channelName, chatterino::Channel::Type::Misc);
        getTimer(channel, 25, false);

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
    func(this->automodChannel);
}

std::shared_ptr<Channel> TwitchIrcServer::getChannelOrEmptyByID(
    const QString &channelId)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (const auto &weakChannel : this->channels)
    {
        auto channel = weakChannel.lock();
        if (!channel)
        {
            continue;
        }

        auto twitchChannel = std::dynamic_pointer_cast<TwitchChannel>(channel);
        if (!twitchChannel)
        {
            continue;
        }

        if (twitchChannel->roomId() == channelId &&
            twitchChannel->getName().count(':') < 2)
        {
            return twitchChannel;
        }
    }

    return Channel::getEmpty();
}

bool TwitchIrcServer::prepareToSend(
    const std::shared_ptr<TwitchChannel> &channel)
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
            channel->addSystemMessage("You are sending messages too quickly.");

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
            channel->addSystemMessage("You are sending too many messages.");

            this->lastErrorTimeAmount_ = now;
        }
        return false;
    }

    lastMessage.push(now);
    return true;
}

void TwitchIrcServer::onMessageSendRequested(
    const std::shared_ptr<TwitchChannel> &channel, const QString &message,
    bool &sent)
{
    sent = false;

    bool canSend = this->prepareToSend(channel);
    if (!canSend)
    {
        return;
    }

    if (shouldSendHelixChat())
    {
        sendHelixMessage(channel, message);
    }
    else
    {
        this->sendMessage(channel->getName(), message);
    }

    sent = true;
}

void TwitchIrcServer::onReplySendRequested(
    const std::shared_ptr<TwitchChannel> &channel, const QString &message,
    const QString &replyId, bool &sent)
{
    sent = false;

    bool canSend = this->prepareToSend(channel);
    if (!canSend)
    {
        return;
    }

    if (shouldSendHelixChat())
    {
        sendHelixMessage(channel, message, replyId);
    }
    else
    {
        this->sendRawMessage("@reply-parent-msg-id=" + replyId + " PRIVMSG #" +
                             channel->getName() + " :" + message);
    }
    sent = true;
}

const IndirectChannel &TwitchIrcServer::getWatchingChannel() const
{
    return this->watchingChannel;
}

void TwitchIrcServer::setWatchingChannel(ChannelPtr newWatchingChannel)
{
    assertInGuiThread();

    this->watchingChannel.reset(newWatchingChannel);
}

ChannelPtr TwitchIrcServer::getWhispersChannel() const
{
    return this->whispersChannel;
}

ChannelPtr TwitchIrcServer::getMentionsChannel() const
{
    return this->mentionsChannel;
}

ChannelPtr TwitchIrcServer::getLiveChannel() const
{
    return this->liveChannel;
}

ChannelPtr TwitchIrcServer::getAutomodChannel() const
{
    return this->automodChannel;
}

QString TwitchIrcServer::getLastUserThatWhisperedMe() const
{
    return this->lastUserThatWhisperedMe.get();
}

void TwitchIrcServer::setLastUserThatWhisperedMe(const QString &user)
{
    assertInGuiThread();

    this->lastUserThatWhisperedMe.set(user);
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

void TwitchIrcServer::reloadAllFFZChannelEmotes()
{
    this->forEachChannel([](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->refreshFFZChannelEmotes(false);
        }
    });
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

void TwitchIrcServer::forEachSeventvEmoteSet(
    const QString &emoteSetId, std::function<void(TwitchChannel &)> func)
{
    this->forEachChannel([emoteSetId, func](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get());
            channel->seventvEmoteSetID() == emoteSetId)
        {
            func(*channel);
        }
    });
}
void TwitchIrcServer::forEachSeventvUser(
    const QString &userId, std::function<void(TwitchChannel &)> func)
{
    this->forEachChannel([userId, func](const auto &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get());
            channel->seventvUserID() == userId)
        {
            func(*channel);
        }
    });
}

void TwitchIrcServer::dropSeventvChannel(const QString &userID,
                                         const QString &emoteSetID)
{
    if (!getApp()->getSeventvEventAPI())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->channelMutex);

    // ignore empty values
    bool skipUser = userID.isEmpty();
    bool skipSet = emoteSetID.isEmpty();

    bool foundUser = skipUser;
    bool foundSet = skipSet;
    for (std::weak_ptr<Channel> &weak : this->channels)
    {
        ChannelPtr chan = weak.lock();
        if (!chan)
        {
            continue;
        }

        auto *channel = dynamic_cast<TwitchChannel *>(chan.get());
        if (!foundSet && channel->seventvEmoteSetID() == emoteSetID)
        {
            foundSet = true;
        }
        if (!foundUser && channel->seventvUserID() == userID)
        {
            foundUser = true;
        }

        if (foundSet && foundUser)
        {
            break;
        }
    }

    if (!foundUser)
    {
        getApp()->getSeventvEventAPI()->unsubscribeUser(userID);
    }
    if (!foundSet)
    {
        getApp()->getSeventvEventAPI()->unsubscribeEmoteSet(emoteSetID);
    }
}

void TwitchIrcServer::markChannelsConnected()
{
    this->forEachChannel([](const ChannelPtr &chan) {
        if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
        {
            channel->markConnected();
        }
    });
}

void TwitchIrcServer::addFakeMessage(const QString &data)
{
    assertInGuiThread();

    auto *fakeMessage = Communi::IrcMessage::fromData(
        data.toUtf8(), this->readConnection_.get());

    if (fakeMessage->command() == "PRIVMSG")
    {
        this->privateMessageReceived(
            static_cast<Communi::IrcPrivateMessage *>(fakeMessage));
    }
    else
    {
        this->readConnectionMessageReceived(fakeMessage);
    }
}

void TwitchIrcServer::addGlobalSystemMessage(const QString &messageText)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    MessageBuilder b(systemMessage, messageText);
    auto message = b.release();

    for (std::weak_ptr<Channel> &weak : this->channels.values())
    {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan)
        {
            continue;
        }

        chan->addMessage(message, MessageContext::Original);
    }
}

void TwitchIrcServer::forEachChannel(std::function<void(ChannelPtr)> func)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (std::weak_ptr<Channel> &weak : this->channels.values())
    {
        ChannelPtr chan = weak.lock();
        if (!chan)
        {
            continue;
        }

        func(chan);
    }
}

void TwitchIrcServer::connect()
{
    assertInGuiThread();

    this->disconnect();

    this->initializeConnection(this->writeConnection_.get(),
                               ConnectionType::Write);
    this->initializeConnection(this->readConnection_.get(),
                               ConnectionType::Read);
}

void TwitchIrcServer::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    this->readConnection_->close();
    this->writeConnection_->close();
}

void TwitchIrcServer::sendMessage(const QString &channelName,
                                  const QString &message)
{
    this->sendRawMessage("PRIVMSG #" + channelName + " :" + message);
}

void TwitchIrcServer::sendRawMessage(const QString &rawMessage)
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    this->writeConnection_->sendRaw(rawMessage);
}

ChannelPtr TwitchIrcServer::getOrAddChannel(const QString &dirtyChannelName)
{
    auto channelName = cleanChannelName(dirtyChannelName);

    // try get channel
    ChannelPtr chan = this->getChannelOrEmpty(channelName);
    if (chan != Channel::getEmpty())
    {
        return chan;
    }

    std::lock_guard<std::mutex> lock(this->channelMutex);

    // value doesn't exist
    chan = this->createChannel(channelName);
    if (!chan)
    {
        return Channel::getEmpty();
    }

    this->channels.insert(channelName, chan);
    this->connections_.managedConnect(chan->destroyed, [this, channelName] {
        // fourtf: issues when the server itself is destroyed

        qCDebug(chatterinoIrc) << "[TwitchIrcServer::addChannel]" << channelName
                               << "was destroyed";
        this->channels.remove(channelName);

        if (this->readConnection_)
        {
            this->readConnection_->sendRaw("PART #" + channelName);
        }
    });

    // join IRC channel
    {
        std::lock_guard<std::mutex> lock2(this->connectionMutex_);

        if (this->readConnection_)
        {
            if (this->readConnection_->isConnected())
            {
                this->joinBucket_->send(channelName);
            }
        }
    }

    return chan;
}

ChannelPtr TwitchIrcServer::getChannelOrEmpty(const QString &dirtyChannelName)
{
    auto channelName = cleanChannelName(dirtyChannelName);

    std::lock_guard<std::mutex> lock(this->channelMutex);

    // try get special channel
    ChannelPtr chan = this->getCustomChannel(channelName);
    if (chan)
    {
        return chan;
    }

    // value exists
    auto it = this->channels.find(channelName);
    if (it != this->channels.end())
    {
        chan = it.value().lock();

        if (chan)
        {
            return chan;
        }
    }

    return Channel::getEmpty();
}

void TwitchIrcServer::open(ConnectionType type)
{
    std::lock_guard<std::mutex> lock(this->connectionMutex_);

    if (type == ConnectionType::Write)
    {
        this->writeConnection_->open();
    }
    if (type == ConnectionType::Read)
    {
        this->readConnection_->open();
    }
}

}  // namespace chatterino
