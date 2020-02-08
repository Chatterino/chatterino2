#pragma once

#include <pajlada/signals/signal.hpp>

#include <IrcConnection>
#include <QTimer>

namespace chatterino {

class IrcConnection : public Communi::IrcConnection
{
public:
    IrcConnection(QObject *parent = nullptr);

    pajlada::Signals::NoArgSignal reconnectRequested;

private:
    QTimer pingTimer_;
    QTimer reconnectTimer_;
    std::atomic<bool> recentlyReceivedMessage_{true};

    // waitingForPong_ is set to true when we send a PING message, and back to false when we receive the matching PONG response
    std::atomic<bool> waitingForPong_{false};
};

}  // namespace chatterino
