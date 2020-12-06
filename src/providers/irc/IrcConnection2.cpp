#include "IrcConnection2.hpp"

#include "common/Version.hpp"

namespace chatterino {

namespace {

    const auto payload = QString("chatterino/" CHATTERINO_VERSION);

}  // namespace

IrcConnection::IrcConnection(QObject *parent)
    : Communi::IrcConnection(parent)
{
    // send ping every x seconds
    this->pingTimer_.setInterval(5000);
    this->pingTimer_.start();
    QObject::connect(&this->pingTimer_, &QTimer::timeout, [this] {
        if (this->isConnected())
        {
            if (this->waitingForPong_.load())
            {
                // Not sending another ping as we haven't received the matching pong yet
                return;
            }

            if (!this->recentlyReceivedMessage_.load())
            {
                this->sendRaw("PING " + payload);
                this->reconnectTimer_.start();
                this->waitingForPong_ = true;
            }
            this->recentlyReceivedMessage_ = false;
        }
    });

    // reconnect after x seconds without receiving a message
    this->reconnectTimer_.setInterval(5000);
    this->reconnectTimer_.setSingleShot(true);
    QObject::connect(&this->reconnectTimer_, &QTimer::timeout, [this] {
        if (this->isConnected())
        {
            reconnectRequested.invoke();
        }
    });

    QObject::connect(this, &Communi::IrcConnection::connected, this, [this] {
        this->waitingForPong_ = false;
    });

    QObject::connect(this, &Communi::IrcConnection::pongMessageReceived,
                     [this](Communi::IrcPongMessage *message) {
                         if (message->argument() == payload)
                         {
                             this->waitingForPong_ = false;
                         }
                     });

    QObject::connect(this, &Communi::IrcConnection::messageReceived,
                     [this](Communi::IrcMessage *) {
                         this->recentlyReceivedMessage_ = true;

                         if (this->reconnectTimer_.isActive())
                         {
                             this->reconnectTimer_.stop();
                         }
                     });
}

}  // namespace chatterino
