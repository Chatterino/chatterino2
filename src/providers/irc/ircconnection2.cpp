#include "ircconnection2.hpp"

namespace chatterino {
namespace providers {
namespace irc {

IrcConnection::IrcConnection(QObject *parent)
    : Communi::IrcConnection(parent)
{
    this->pingTimer_.setInterval(5000);
    this->pingTimer_.start();
    QObject::connect(&this->pingTimer_, &QTimer::timeout, [this] {
        if (!this->recentlyReceivedMessage_.load()) {
            this->sendRaw("PING");
            this->reconnectTimer_.start();
        }
        this->recentlyReceivedMessage_ = false;
    });

    this->reconnectTimer_.setInterval(5000);
    this->reconnectTimer_.setSingleShot(true);
    QObject::connect(&this->reconnectTimer_, &QTimer::timeout,
                     [this] { reconnectRequested.invoke(); });

    QObject::connect(this, &Communi::IrcConnection::messageReceived, [this](Communi::IrcMessage *) {
        this->recentlyReceivedMessage_ = true;

        if (this->reconnectTimer_.isActive()) {
            this->reconnectTimer_.stop();
        }
    });
}

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
