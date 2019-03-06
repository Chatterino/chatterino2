#include "AbstractIrcServer.hpp"

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "util/Log.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

#include <QCoreApplication>

namespace chatterino
{
    const int RECONNECT_BASE_INTERVAL = 2000;
    // 60 falloff counter means it will try to reconnect at most every 60*2
    // seconds
    const int MAX_FALLOFF_COUNTER = 60;

    AbstractIrcServer::AbstractIrcServer()
    {
        // Initialize the connections
        this->writeConnection_.reset(new IrcConnection);
        this->writeConnection_->moveToThread(
            QCoreApplication::instance()->thread());

        QObject::connect(this->writeConnection_.get(),
            &Communi::IrcConnection::messageReceived,
            [this](auto msg) { this->writeConnectionMessageReceived(msg); });

        // Listen to read connection message signals
        this->readConnection_.reset(new IrcConnection);
        this->readConnection_->moveToThread(
            QCoreApplication::instance()->thread());

        QObject::connect(this->readConnection_.get(),
            &Communi::IrcConnection::messageReceived,
            [this](auto msg) { this->messageReceived(msg); });
        QObject::connect(this->readConnection_.get(),
            &Communi::IrcConnection::privateMessageReceived,
            [this](auto msg) { this->privateMessageReceived(msg); });
        QObject::connect(this->readConnection_.get(),
            &Communi::IrcConnection::connected,
            [this] { this->onConnected(); });
        QObject::connect(this->readConnection_.get(),
            &Communi::IrcConnection::disconnected,
            [this] { this->onDisconnected(); });
        QObject::connect(this->readConnection_.get(),
            &Communi::IrcConnection::socketError,
            [this] { this->onSocketError(); });

        // listen to reconnect request
        this->readConnection_->reconnectRequested.connect(
            [this] { this->connect(); });
        //    this->writeConnection->reconnectRequested.connect([this] {
        //    this->connect(); });
        this->reconnectTimer_.setInterval(RECONNECT_BASE_INTERVAL);
        this->reconnectTimer_.setSingleShot(true);
        QObject::connect(&this->reconnectTimer_, &QTimer::timeout, [this] {
            this->reconnectTimer_.setInterval(
                RECONNECT_BASE_INTERVAL * this->falloffCounter_);

            this->falloffCounter_ =
                std::min(MAX_FALLOFF_COUNTER, this->falloffCounter_ + 1);

            if (!this->readConnection_->isConnected())
            {
                log("Trying to reconnect... {}", this->falloffCounter_);
                this->connect();
            }
        });
    }

    void AbstractIrcServer::connect()
    {
        this->disconnect();

        bool separateWriteConnection = this->hasSeparateWriteConnection();

        if (separateWriteConnection)
        {
            this->initializeConnection(
                this->writeConnection_.get(), false, true);
            this->initializeConnection(
                this->readConnection_.get(), true, false);
        }
        else
        {
            this->initializeConnection(this->readConnection_.get(), true, true);
        }

        // fourtf: this should be asynchronous
        {
            std::lock_guard<std::mutex> lock1(this->connectionMutex_);
            std::lock_guard<std::mutex> lock2(this->channelMutex);

            for (std::weak_ptr<Channel>& weak : this->channels.values())
            {
                if (auto channel = std::shared_ptr<Channel>(weak.lock()))
                {
                    this->readConnection_->sendRaw(
                        "JOIN #" + channel->getName());
                }
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

    void AbstractIrcServer::sendMessage(
        const QString& channelName, const QString& message)
    {
        this->sendRawMessage("PRIVMSG #" + channelName + " :" + message);
    }

    void AbstractIrcServer::sendRawMessage(const QString& rawMessage)
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
        Communi::IrcMessage* message)
    {
    }

    std::shared_ptr<Channel> AbstractIrcServer::getOrAddChannel(
        const QString& dirtyChannelName)
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

        QString clojuresInCppAreShit = channelName;

        this->channels.insert(channelName, chan);
        chan->destroyed.connect([this, clojuresInCppAreShit] {
            // fourtf: issues when the server itself is destroyed

            log("[AbstractIrcServer::addChannel] {} was destroyed",
                clojuresInCppAreShit);
            this->channels.remove(clojuresInCppAreShit);

            if (this->readConnection_)
            {
                this->readConnection_->sendRaw("PART #" + clojuresInCppAreShit);
            }

            if (this->writeConnection_)
            {
                this->writeConnection_->sendRaw(
                    "PART #" + clojuresInCppAreShit);
            }
        });

        // join irc channel
        {
            std::lock_guard<std::mutex> lock2(this->connectionMutex_);

            if (this->readConnection_)
            {
                this->readConnection_->sendRaw("JOIN #" + channelName);
            }

            if (this->writeConnection_)
            {
                this->writeConnection_->sendRaw("JOIN #" + channelName);
            }
        }

        return chan;
    }

    std::shared_ptr<Channel> AbstractIrcServer::getChannelOrEmpty(
        const QString& dirtyChannelName)
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

    void AbstractIrcServer::onConnected()
    {
        std::lock_guard<std::mutex> lock(this->channelMutex);

        auto connected = makeSystemMessage("connected");
        connected->flags.set(MessageFlag::ConnectedMessage);
        connected->flags.set(MessageFlag::Centered);
        auto reconnected = makeSystemMessage("reconnected");
        reconnected->flags.set(MessageFlag::ConnectedMessage);

        for (std::weak_ptr<Channel>& weak : this->channels.values())
        {
            std::shared_ptr<Channel> chan = weak.lock();
            if (!chan)
            {
                continue;
            }

            LimitedQueueSnapshot<MessagePtr> snapshot =
                chan->getMessageSnapshot();

            bool replaceMessage =
                snapshot.size() > 0 && snapshot[snapshot.size() - 1]->flags.has(
                                           MessageFlag::DisconnectedMessage);

            if (replaceMessage)
            {
                chan->replaceMessage(
                    snapshot[snapshot.size() - 1], reconnected);
                continue;
            }

            chan->addMessage(connected);
        }

        this->falloffCounter_ = 1;
    }

    void AbstractIrcServer::onDisconnected()
    {
        std::lock_guard<std::mutex> lock(this->channelMutex);

        MessageBuilder b(systemMessage, "disconnected");
        b->flags.set(MessageFlag::DisconnectedMessage);
        auto disconnected = b.release();

        for (std::weak_ptr<Channel>& weak : this->channels.values())
        {
            std::shared_ptr<Channel> chan = weak.lock();
            if (!chan)
            {
                continue;
            }

            chan->addMessage(disconnected);
        }
    }

    void AbstractIrcServer::onSocketError()
    {
        this->reconnectTimer_.start();
    }

    std::shared_ptr<Channel> AbstractIrcServer::getCustomChannel(
        const QString& channelName)
    {
        return nullptr;
    }

    QString AbstractIrcServer::cleanChannelName(const QString& dirtyChannelName)
    {
        return dirtyChannelName;
    }

    void AbstractIrcServer::addFakeMessage(const QString& data)
    {
        auto fakeMessage = Communi::IrcMessage::fromData(
            data.toUtf8(), this->readConnection_.get());

        if (fakeMessage->command() == "PRIVMSG")
        {
            this->privateMessageReceived(
                static_cast<Communi::IrcPrivateMessage*>(fakeMessage));
        }
        else
        {
            this->messageReceived(fakeMessage);
        }
    }

    void AbstractIrcServer::privateMessageReceived(
        Communi::IrcPrivateMessage* message)
    {
    }

    void AbstractIrcServer::messageReceived(Communi::IrcMessage* message)
    {
    }

    void AbstractIrcServer::forEachChannel(std::function<void(ChannelPtr)> func)
    {
        std::lock_guard<std::mutex> lock(this->channelMutex);

        for (std::weak_ptr<Channel>& weak : this->channels.values())
        {
            std::shared_ptr<Channel> chan = weak.lock();
            if (!chan)
            {
                continue;
            }

            func(chan);
        }
    }

}  // namespace chatterino
