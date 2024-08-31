#include "providers/irc/IrcConnection2.hpp"

#include "common/QLogging.hpp"
#include "common/Version.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace chatterino {

namespace {

    const auto payload = "chatterino/" + CHATTERINO_VERSION;

}  // namespace

IrcConnection::IrcConnection(QObject *parent)
    : Communi::IrcConnection(parent)
{
    // Log connection errors for ease-of-debugging
    QObject::connect(this, &Communi::IrcConnection::socketError, this,
                     [](QAbstractSocket::SocketError error) {
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
    this->lastPing_ = std::chrono::system_clock::now();
    QObject::connect(&this->pingTimer_, &QTimer::timeout, [this] {
        if (this->isConnected())
        {
            if (this->recentlyReceivedMessage_.load())
            {
                // If we're still receiving messages, all is well
                this->recentlyReceivedMessage_ = false;
                this->waitingForPong_ = false;

                // Check if we got invoked too late (e.g. due to a sleep)
                auto now = std::chrono::system_clock::now();
                auto elapsed = now - this->lastPing_;
                if (elapsed < 3 * 5000ms)
                {
                    this->heartbeat.invoke();
                }
                else
                {
                    qCDebug(chatterinoIrc).nospace()
                        << "Got late ping (skipping heartbeat): "
                        << std::chrono::duration_cast<
                               std::chrono::milliseconds>(elapsed)
                               .count()
                        << "ms";
                }
                this->lastPing_ = now;

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
                         this->recentlyReceivedMessage_ = true;

                         if (message->command() == "372")  // MOTD
                         {
                             this->reconnectBackoff_.reset();
                         }
                     });
}

IrcConnection::~IrcConnection()
{
    // Prematurely disconnect all QObject connections
    this->disconnect();
}

void IrcConnection::smartReconnect()
{
    if (this->reconnectTimer_.isActive())
    {
        // Ignore this reconnect request, we already have a reconnect request queued up
        return;
    }

    auto delay = this->reconnectBackoff_.next();
    qCDebug(chatterinoIrc) << "Reconnecting in" << delay.count() << "ms";
    this->reconnectTimer_.start(delay);
}

void IrcConnection::open()
{
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
