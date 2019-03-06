#pragma once

#include <pajlada/signals/signal.hpp>

#include <IrcConnection>
#include <QTimer>

namespace chatterino
{
    class IrcConnection : public Communi::IrcConnection
    {
    public:
        IrcConnection(QObject* parent = nullptr);

        pajlada::Signals::NoArgSignal reconnectRequested;

    private:
        QTimer pingTimer_;
        QTimer reconnectTimer_;
        std::atomic<bool> recentlyReceivedMessage_{true};
    };

}  // namespace chatterino
