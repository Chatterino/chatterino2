#pragma once

#include "util/ExponentialBackoff.hpp"

#include <IrcConnection>
#include <pajlada/signals/signal.hpp>
#include <QTimer>

#include <chrono>

namespace chatterino {

class IrcConnection : public Communi::IrcConnection
{
public:
    IrcConnection(QObject *parent = nullptr);
    ~IrcConnection() override;

    // Signal to notify that we're unexpectedly no longer connected, either due
    // to a connection error or if we think we've timed out. It's up to the
    // receiver to trigger a reconnect, if desired
    pajlada::Signals::Signal<bool> connectionLost;

    // Signal to indicate the connection is still healthy
    pajlada::Signals::NoArgSignal heartbeat;

    // Request a reconnect with a minimum interval between attempts.
    // This won't violate RECONNECT_MIN_INTERVAL
    void smartReconnect();

    virtual void open();
    virtual void close();

private:
    QTimer pingTimer_;
    QTimer reconnectTimer_;
    std::atomic<bool> recentlyReceivedMessage_{true};
    std::chrono::time_point<std::chrono::system_clock> lastPing_;

    // Reconnect with a base delay of 1 second and max out at 1 second * (2^(5-1)) (i.e. 16 seconds)
    ExponentialBackoff<5> reconnectBackoff_{std::chrono::milliseconds{1000}};

    std::atomic<bool> expectConnectionLoss_{false};

    // waitingForPong_ is set to true when we send a PING message, and back to
    // false when we receive the matching PONG response
    std::atomic<bool> waitingForPong_{false};
};

}  // namespace chatterino
