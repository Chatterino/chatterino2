#include "AbstractIrcServer.hpp"

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/QLogging.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

#include <QCoreApplication>

namespace chatterino {

const int RECONNECT_BASE_INTERVAL = 2000;
// 60 falloff counter means it will try to reconnect at most every 60*2 seconds
const int MAX_FALLOFF_COUNTER = 60;

// Ratelimits for joinBucket_
const int JOIN_RATELIMIT_BUDGET = 18;
const int JOIN_RATELIMIT_COOLDOWN = 12500;

AbstractIrcServer::AbstractIrcServer()
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
    this->writeConnection_->connectionLost.connect([this](bool timeout) {
        qCDebug(chatterinoIrc)
            << "Write connection reconnect requested. Timeout:" << timeout;
        this->writeConnection_->smartReconnect.invoke();
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
    this->readConnection_->connectionLost.connect([this](bool timeout) {
        qCDebug(chatterinoIrc)
            << "Read connection reconnect requested. Timeout:" << timeout;
        if (timeout)
        {
            // Show additional message since this is going to interrupt a
            // connection that is still "connected"
            this->addGlobalSystemMessage(
                "Server connection timed out, reconnecting");
        }
        this->readConnection_->smartReconnect.invoke();
    });
}

void AbstractIrcServer::initializeIrc()
{
    assert(!this->initialized_);

    if (this->hasSeparateWriteConnection())
    {
        this->initializeConnectionSignals(this->writeConnection_.get(),
                                          ConnectionType::Write);
        this->initializeConnectionSignals(this->readConnection_.get(),
                                          ConnectionType::Read);
    }
    else
    {
        this->initializeConnectionSignals(this->readConnection_.get(),
                                          ConnectionType::Both);
    }

    this->initialized_ = true;
}

void AbstractIrcServer::connect()
{
    assert(this->initialized_);

    this->disconnect();

    if (this->hasSeparateWriteConnection())
    {
        this->initializeConnection(this->writeConnection_.get(), Write);
        this->initializeConnection(this->readConnection_.get(), Read);
    }
    else
    {
        this->initializeConnection(this->readConnection_.get(), Both);
    }
}

void AbstractIrcServer::open(ConnectionType type)
{
    std::lock_guard<std::mutex> lock(this->connectionMutex_);

    if (type == Write)
    {
        this->writeConnection_->open();
    }
    if (type & Read)
    {
        this->readConnection_->open();
    }
}

void AbstractIrcServer::addGlobalSystemMessage(const QString &messageText)
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

        chan->addMessage(message);
    }
}

void AbstractIrcServer::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    this->readConnection_->close();
    if (this->hasSeparateWriteConnection())
    {
        this->writeConnection_->close();
    }
}

void AbstractIrcServer::sendMessage(const QString &channelName,
                                    const QString &message)
{
    this->sendRawMessage("PRIVMSG #" + channelName + " :" + message);
}

void AbstractIrcServer::sendRawMessage(const QString &rawMessage)
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    if (this->hasSeparateWriteConnection())
    {
        this->writeConnection_->sendRaw(rawMessage);
    }
    else
    {
        this->readConnection_->sendRaw(rawMessage);
    }
}

void AbstractIrcServer::writeConnectionMessageReceived(
    Communi::IrcMessage *message)
{
    (void)message;
}

ChannelPtr AbstractIrcServer::getOrAddChannel(const QString &dirtyChannelName)
{
    auto channelName = this->cleanChannelName(dirtyChannelName);

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
    this->connections_.emplace_back(
        chan->destroyed.connect([this, channelName] {
            // fourtf: issues when the server itself is destroyed

            qCDebug(chatterinoIrc) << "[AbstractIrcServer::addChannel]"
                                   << channelName << "was destroyed";
            this->channels.remove(channelName);

            if (this->readConnection_)
            {
                this->readConnection_->sendRaw("PART #" + channelName);
            }
        }));

    // join irc channel
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

ChannelPtr AbstractIrcServer::getChannelOrEmpty(const QString &dirtyChannelName)
{
    auto channelName = this->cleanChannelName(dirtyChannelName);

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

std::vector<std::weak_ptr<Channel>> AbstractIrcServer::getChannels()
{
    std::lock_guard lock(this->channelMutex);
    std::vector<std::weak_ptr<Channel>> channels;

    for (auto &&weak : this->channels.values())
    {
        channels.push_back(weak);
    }

    return channels;
}

void AbstractIrcServer::onReadConnected(IrcConnection *connection)
{
    (void)connection;

    std::lock_guard lock(this->channelMutex);

    // join channels
    for (auto &&weak : this->channels)
    {
        if (auto channel = weak.lock())
        {
            this->joinBucket_->send(channel->getName());
        }
    }

    // connected/disconnected message
    auto connectedMsg = makeSystemMessage("connected");
    connectedMsg->flags.set(MessageFlag::ConnectedMessage);
    auto reconnected = makeSystemMessage("reconnected");
    reconnected->flags.set(MessageFlag::ConnectedMessage);

    for (std::weak_ptr<Channel> &weak : this->channels.values())
    {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan)
        {
            continue;
        }

        LimitedQueueSnapshot<MessagePtr> snapshot = chan->getMessageSnapshot();

        bool replaceMessage =
            snapshot.size() > 0 && snapshot[snapshot.size() - 1]->flags.has(
                                       MessageFlag::DisconnectedMessage);

        if (replaceMessage)
        {
            chan->replaceMessage(snapshot[snapshot.size() - 1], reconnected);
            continue;
        }

        chan->addMessage(connectedMsg);
    }

    this->falloffCounter_ = 1;
}

void AbstractIrcServer::onWriteConnected(IrcConnection *connection)
{
    (void)connection;
}

void AbstractIrcServer::onDisconnected()
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

        chan->addMessage(disconnectedMsg);
    }
}

std::shared_ptr<Channel> AbstractIrcServer::getCustomChannel(
    const QString &channelName)
{
    (void)channelName;
    return nullptr;
}

QString AbstractIrcServer::cleanChannelName(const QString &dirtyChannelName)
{
    if (dirtyChannelName.startsWith('#'))
        return dirtyChannelName.mid(1);
    else
        return dirtyChannelName;
}

void AbstractIrcServer::addFakeMessage(const QString &data)
{
    auto fakeMessage = Communi::IrcMessage::fromData(
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

void AbstractIrcServer::privateMessageReceived(
    Communi::IrcPrivateMessage *message)
{
    (void)message;
}

void AbstractIrcServer::readConnectionMessageReceived(
    Communi::IrcMessage *message)
{
}

void AbstractIrcServer::forEachChannel(std::function<void(ChannelPtr)> func)
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

}  // namespace chatterino
