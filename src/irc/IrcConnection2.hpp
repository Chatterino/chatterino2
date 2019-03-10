#pragma once

#include <IrcConnection>
#include <QTimer>

namespace chatterino
{
    class IrcConnection : public Communi::IrcConnection
    {
        Q_OBJECT

    public:
        IrcConnection(QObject* parent = nullptr);

    signals:
        void reconnectRequested();

    private:
        QTimer pingTimer_;
        QTimer reconnectTimer_;
        std::atomic<bool> recentlyReceivedMessage_{true};
    };
}  // namespace chatterino
