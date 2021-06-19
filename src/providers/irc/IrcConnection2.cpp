#include "IrcConnection2.hpp"

#include "common/QLogging.hpp"
#include "common/Version.hpp"

namespace chatterino {

// The minimum interval between attempting to establish a new connection
const int RECONNECT_MIN_INTERVAL = 15000;

namespace {

    const auto payload = QString("chatterino/" CHATTERINO_VERSION);

}  // namespace

IrcConnection::IrcConnection(QObject *parent)
    : Communi::IrcConnection(parent)
{
    // Log connection errors for ease-of-debugging
    QObject::connect(this, &Communi::IrcConnection::socketError, this,
                     [this](QAbstractSocket::SocketError error) {
                         qCDebug(chatterinoIrc) << "Connection error:" << error;
                     });

    QObject::connect(
        this, &Communi::IrcConnection::socketStateChanged, this,
        [this](QAbstractSocket::SocketState state) {
            if (state == QAbstractSocket::UnconnectedState)
            {
                this->pingTimer_.stop();

                // The socket will enter unconnected state both in case of
                // socket error (including failures to connect) and regular
                // disconnects. We signal that the connection was lost if this
                // was not the result of us calling `close`.
                if (!this->expectConnectionLoss_.load())
                {
                    this->connectionLost.invoke(false);
                }
            }
        });

    // Schedule a reconnect that won't violate RECONNECT_MIN_INTERVAL
    this->smartReconnect.connect([this] {
        if (this->reconnectTimer_.isActive())
        {
            return;
        }

        auto delta =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - this->lastConnected_)
                .count();
        delta = delta < RECONNECT_MIN_INTERVAL
                    ? (RECONNECT_MIN_INTERVAL - delta)
                    : 10;
        qCDebug(chatterinoIrc) << "Reconnecting in" << delta << "ms";
        this->reconnectTimer_.start(delta);
    });

    this->reconnectTimer_.setInterval(RECONNECT_MIN_INTERVAL);
    this->reconnectTimer_.setSingleShot(true);
    QObject::connect(&this->reconnectTimer_, &QTimer::timeout, [this] {
        if (this->isConnected())
        {
            // E.g. user manually reconnecting doesn't cancel this path, so
            // just ignore
            qCDebug(chatterinoIrc) << "Reconnect: already reconnected";
        }
        else
        {
            qCDebug(chatterinoIrc) << "Reconnecting";
            this->open();
        }
    });

    // Send ping every x seconds
    this->pingTimer_.setInterval(5000);
    this->pingTimer_.start();
    QObject::connect(&this->pingTimer_, &QTimer::timeout, [this] {
        if (this->isConnected())
        {
            if (this->recentlyReceivedMessage_.load())
            {
                // If we're still receiving messages, all is well
                this->recentlyReceivedMessage_ = false;
                this->waitingForPong_ = false;
                return;
            }

            if (this->waitingForPong_.load())
            {
                // The remote server did not send a PONG fast enough; close the
                // connection
                this->close();
                this->connectionLost.invoke(true);
            }
            else
            {
                this->sendRaw("PING " + payload);
                this->waitingForPong_ = true;
            }
        }
    });

    QObject::connect(this, &Communi::IrcConnection::connected, this, [this] {
        this->pingTimer_.start();
    });

    QObject::connect(this, &Communi::IrcConnection::pongMessageReceived,
                     [this](Communi::IrcPongMessage *message) {
                         if (message->argument() == payload)
                         {
                             this->waitingForPong_ = false;
                         }
                     });

    QObject::connect(this, &Communi::IrcConnection::messageReceived,
                     [this](Communi::IrcMessage *message) {
                         // This connection is probably still alive
                         this->recentlyReceivedMessage_ = true;
                     });
}

void IrcConnection::open()
{
    // Accurately track the time a connection was opened
    this->lastConnected_ = std::chrono::steady_clock::now();
    this->expectConnectionLoss_ = false;
    this->waitingForPong_ = false;
    this->recentlyReceivedMessage_ = false;
    Communi::IrcConnection::open();
}

void IrcConnection::close()
{
    this->expectConnectionLoss_ = true;
    Communi::IrcConnection::close();
}

}  // namespace chatterino
