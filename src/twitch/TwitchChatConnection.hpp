#pragma once

#include <QObject>

namespace chatterino
{
    namespace
    {
        class Private;
    }

    class IrcConnection;

    class TwitchChatConnection : public QObject
    {
        Q_OBJECT

    public:
        TwitchChatConnection(QObject* parent = nullptr);
        ~TwitchChatConnection();

        void connect();
        void join(const QString&);
        void part(const QString&);

        IrcConnection* getReadConnection();
        IrcConnection* getWriteConnection();

    private:
        Private* this_;
    };
}  // namespace chatterino
