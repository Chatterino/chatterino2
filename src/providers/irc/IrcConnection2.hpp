#pragma once

#include <pajlada/signals/signal.hpp>

#include <IrcConnection>
#include <QTimer>

namespace chatterino {
namespace providers {
namespace irc {

class IrcConnection : public Communi::IrcConnection
{
public:
    IrcConnection(QObject *parent = nullptr);

    pajlada::Signals::NoArgSignal reconnectRequested;

private:
    QTimer pingTimer_;
    QTimer reconnectTimer_;
    std::atomic<bool> recentlyReceivedMessage_{true};
};

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
