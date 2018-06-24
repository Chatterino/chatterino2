#include "abstractircserver.hpp"

#include "common.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"

#include <QCoreApplication>

using namespace chatterino::messages;

namespace chatterino {
namespace providers {
namespace irc {

AbstractIrcServer::AbstractIrcServer()
{
    // Initialize the connections
    this->writeConnection_.reset(new IrcConnection);
    this->writeConnection_->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->writeConnection_.get(), &Communi::IrcConnection::messageReceived,
                     [this](auto msg) { this->writeConnectionMessageReceived(msg); });

    // Listen to read connection message signals
    this->readConnection_.reset(new IrcConnection);
    this->readConnection_->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->readConnection_.get(), &Communi::IrcConnection::messageReceived,
                     [this](auto msg) { this->messageReceived(msg); });
    QObject::connect(this->readConnection_.get(), &Communi::IrcConnection::privateMessageReceived,
                     [this](auto msg) { this->privateMessageReceived(msg); });
    QObject::connect(this->readConnection_.get(), &Communi::IrcConnection::connected,
                     [this] { this->onConnected(); });
    QObject::connect(this->readConnection_.get(), &Communi::IrcConnection::disconnected,
                     [this] { this->onDisconnected(); });

    // listen to reconnect request
    this->readConnection_->reconnectRequested.connect([this] { this->connect(); });
    //    this->writeConnection->reconnectRequested.connect([this] { this->connect(); });
}

IrcConnection *AbstractIrcServer::getReadConnection() const
{
    return this->readConnection_.get();
}

IrcConnection *AbstractIrcServer::getWriteConnection() const
{
    return this->writeConnection_.get();
}

void AbstractIrcServer::connect()
{
    this->disconnect();

    //    if (this->hasSeperateWriteConnection()) {
    this->initializeConnection(this->writeConnection_.get(), false, true);
    this->initializeConnection(this->readConnection_.get(), true, false);
    //    } else {
    //        this->initializeConnection(this->readConnection.get(), true, true);
    //    }

    // fourtf: this should be asynchronous
    {
        std::lock_guard<std::mutex> lock1(this->connectionMutex_);
        std::lock_guard<std::mutex> lock2(this->channelMutex);

        for (std::weak_ptr<Channel> &weak : this->channels.values()) {
            std::shared_ptr<Channel> chan = weak.lock();
            if (!chan) {
                continue;
            }

            this->writeConnection_->sendRaw("JOIN #" + chan->name);
            this->readConnection_->sendRaw("JOIN #" + chan->name);
        }

        this->writeConnection_->open();
        this->readConnection_->open();
    }

    //    this->onConnected();
    // possbile event: started to connect
}

void AbstractIrcServer::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    this->readConnection_->close();
    this->writeConnection_->close();
}

void AbstractIrcServer::sendMessage(const QString &channelName, const QString &message)
{
    std::lock_guard<std::mutex> locker(this->connectionMutex_);

    // fourtf: trim the message if it's sent from twitch chat

    if (this->writeConnection_) {
        this->writeConnection_->sendRaw("PRIVMSG #" + channelName + " :" + message);
    }
}

void AbstractIrcServer::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
}

std::shared_ptr<Channel> AbstractIrcServer::getOrAddChannel(const QString &dirtyChannelName)
{
    auto channelName = this->cleanChannelName(dirtyChannelName);

    // try get channel
    ChannelPtr chan = this->getChannelOrEmpty(channelName);
    if (chan != Channel::getEmpty()) {
        return chan;
    }

    std::lock_guard<std::mutex> lock(this->channelMutex);

    // value doesn't exist
    chan = this->createChannel(channelName);
    if (!chan) {
        return Channel::getEmpty();
    }

    QString clojuresInCppAreShit = channelName;

    this->channels.insert(channelName, chan);
    chan->destroyed.connect([this, clojuresInCppAreShit] {
        // fourtf: issues when the server itself is destroyed

        debug::Log("[AbstractIrcServer::addChannel] {} was destroyed", clojuresInCppAreShit);
        this->channels.remove(clojuresInCppAreShit);

        if (this->readConnection_) {
            this->readConnection_->sendRaw("PART #" + clojuresInCppAreShit);
        }

        if (this->writeConnection_) {
            this->writeConnection_->sendRaw("PART #" + clojuresInCppAreShit);
        }
    });

    // join irc channel
    {
        std::lock_guard<std::mutex> lock2(this->connectionMutex_);

        if (this->readConnection_) {
            this->readConnection_->sendRaw("JOIN #" + channelName);
        }

        if (this->writeConnection_) {
            this->writeConnection_->sendRaw("JOIN #" + channelName);
        }
    }

    return chan;
}

std::shared_ptr<Channel> AbstractIrcServer::getChannelOrEmpty(const QString &dirtyChannelName)
{
    auto channelName = this->cleanChannelName(dirtyChannelName);

    std::lock_guard<std::mutex> lock(this->channelMutex);

    // try get special channel
    ChannelPtr chan = this->getCustomChannel(channelName);
    if (chan) {
        return chan;
    }

    // value exists
    auto it = this->channels.find(channelName);
    if (it != this->channels.end()) {
        chan = it.value().lock();

        if (chan) {
            return chan;
        }
    }

    return Channel::getEmpty();
}

void AbstractIrcServer::onConnected()
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    MessagePtr connMsg = Message::createSystemMessage("connected to chat");
    MessagePtr reconnMsg = Message::createSystemMessage("reconnected to chat");

    for (std::weak_ptr<Channel> &weak : this->channels.values()) {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan) {
            continue;
        }

        LimitedQueueSnapshot<MessagePtr> snapshot = chan->getMessageSnapshot();

        bool replaceMessage =
            snapshot.getLength() > 0 &&
            snapshot[snapshot.getLength() - 1]->flags & Message::DisconnectedMessage;

        if (replaceMessage) {
            chan->replaceMessage(snapshot[snapshot.getLength() - 1], reconnMsg);
            continue;
        }

        chan->addMessage(connMsg);
    }
}

void AbstractIrcServer::onDisconnected()
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    MessagePtr msg = Message::createSystemMessage("disconnected from chat");
    msg->flags |= Message::DisconnectedMessage;

    for (std::weak_ptr<Channel> &weak : this->channels.values()) {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan) {
            continue;
        }

        chan->addMessage(msg);
    }
}

std::shared_ptr<Channel> AbstractIrcServer::getCustomChannel(const QString &channelName)
{
    return nullptr;
}

QString AbstractIrcServer::cleanChannelName(const QString &dirtyChannelName)
{
    return dirtyChannelName;
}

void AbstractIrcServer::addFakeMessage(const QString &data)
{
    auto fakeMessage = Communi::IrcMessage::fromData(data.toUtf8(), this->readConnection_.get());

    if (fakeMessage->command() == "PRIVMSG") {
        this->privateMessageReceived(static_cast<Communi::IrcPrivateMessage *>(fakeMessage));
    } else {
        this->messageReceived(fakeMessage);
    }
}

void AbstractIrcServer::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
}

void AbstractIrcServer::messageReceived(Communi::IrcMessage *message)
{
}

void AbstractIrcServer::forEachChannel(std::function<void(ChannelPtr)> func)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (std::weak_ptr<Channel> &weak : this->channels.values()) {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan) {
            continue;
        }

        func(chan);
    }
}

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
