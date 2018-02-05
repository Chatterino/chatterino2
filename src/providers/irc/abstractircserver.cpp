#include "abstractircserver.hpp"

#include "common.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace providers {
namespace irc {

AbstractIrcServer::AbstractIrcServer()
{
    // Initialize the connections
    this->writeConnection.reset(new Communi::IrcConnection);
    this->writeConnection->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->writeConnection.get(), &Communi::IrcConnection::messageReceived,
                     [this](auto msg) { this->writeConnectionMessageReceived(msg); });

    // Listen to read connection message signals
    this->readConnection.reset(new Communi::IrcConnection);
    this->readConnection->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::messageReceived,
                     [this](auto msg) { this->messageReceived(msg); });
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::privateMessageReceived,
                     [this](auto msg) { this->privateMessageReceived(msg); });
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::connected,
                     [this] { this->onConnected(); });
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::disconnected,
                     [this] { this->onDisconnected(); });
}

Communi::IrcConnection *AbstractIrcServer::getReadConnection() const
{
    return this->readConnection.get();
}

void AbstractIrcServer::connect()
{
    this->disconnect();

    //    if (this->hasSeperateWriteConnection()) {
    this->initializeConnection(this->writeConnection.get(), false, true);
    this->initializeConnection(this->readConnection.get(), true, false);
    //    } else {
    //        this->initializeConnection(this->readConnection.get(), true, true);
    //    }

    // fourtf: this should be asynchronous
    {
        std::lock_guard<std::mutex> lock1(this->connectionMutex);
        std::lock_guard<std::mutex> lock2(this->channelMutex);

        for (std::weak_ptr<Channel> &weak : this->channels.values()) {
            std::shared_ptr<Channel> chan = weak.lock();
            if (!chan) {
                continue;
            }

            this->writeConnection->sendRaw("JOIN #" + chan->name);
            this->readConnection->sendRaw("JOIN #" + chan->name);
        }

        this->writeConnection->open();
        this->readConnection->open();
    }

    this->onConnected();
    this->connected.invoke();
}

void AbstractIrcServer::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex);

    this->readConnection->close();
    this->writeConnection->close();
}

void AbstractIrcServer::sendMessage(const QString &channelName, const QString &message)
{
    std::lock_guard<std::mutex> locker(this->connectionMutex);

    // fourtf: trim the message if it's sent from twitch chat

    if (this->writeConnection) {
        this->writeConnection->sendRaw("PRIVMSG #" + channelName + " :" + message);
    }
}

void AbstractIrcServer::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
}

std::shared_ptr<Channel> AbstractIrcServer::addChannel(const QString &channelName)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    // value exists
    auto it = this->channels.find(channelName);
    if (it != this->channels.end()) {
        std::shared_ptr<Channel> chan = it.value().lock();

        if (chan) {
            return chan;
        }
    }

    // value doesn't exist
    std::shared_ptr<Channel> chan = this->createChannel(channelName);
    if (!chan) {
        return Channel::getEmpty();
    }

    QString clojuresInCppAreShit = channelName;

    this->channels.insert(channelName, chan);
    chan->destroyed.connect([this, clojuresInCppAreShit] {
        // fourtf: issues when the server itself in destroyed

        debug::Log("[AbstractIrcServer::addChannel] {} was destroyed", clojuresInCppAreShit);
        this->channels.remove(clojuresInCppAreShit);
    });

    // join irc channel
    {
        std::lock_guard<std::mutex> lock2(this->connectionMutex);
        if (this->readConnection) {
            this->readConnection->sendRaw("JOIN #" + channelName);
        }

        if (this->writeConnection) {
            this->writeConnection->sendRaw("JOIN #" + channelName);
        }
    }

    return chan;
}

std::shared_ptr<Channel> AbstractIrcServer::getChannel(const QString &channelName)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    // value exists
    auto it = this->channels.find(channelName);
    if (it != this->channels.end()) {
        std::shared_ptr<Channel> chan = it.value().lock();

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
    msg->flags &= Message::DisconnectedMessage;

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

void AbstractIrcServer::addFakeMessage(const QString &data)
{
    auto fakeMessage = Communi::IrcMessage::fromData(data.toUtf8(), this->readConnection.get());

    this->privateMessageReceived(qobject_cast<Communi::IrcPrivateMessage *>(fakeMessage));
}

void AbstractIrcServer::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
}

void AbstractIrcServer::messageReceived(Communi::IrcMessage *message)
{
}
}  // namespace irc
}  // namespace providers
}  // namespace chatterino
