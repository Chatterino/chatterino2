#pragma once

#include <pajlada/signals/signal.hpp>

#include <IrcConnection>
#include <QTimer>
#include <chrono>

namespace chatterino {

class IrcConnection : public Communi::IrcConnection
{
public:
    IrcConnection(QObject *parent = nullptr);

    // Signal to notify that we're no longer connected; signal receiver should
    // trigger a reconnect, if desired
    pajlada::Signals::Signal<bool> connectionLost;

    // Request a reconnect with a minimum interval between attempts.
    pajlada::Signals::NoArgSignal smartReconnect;

    virtual void open();
    virtual void close();

private:
    QTimer pingTimer_;
    QTimer reconnectTimer_;
    std::atomic<bool> recentlyReceivedMessage_{true};
    std::chrono::steady_clock::time_point lastConnected_;

    std::atomic<bool> expectConnectionLoss_{false};

    // waitingForPong_ is set to true when we send a PING message, and back to
    // false when we receive the matching PONG response
    std::atomic<bool> waitingForPong_{false};
};

}  // namespace chatterino
