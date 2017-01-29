#ifndef IRCMANAGER_H
#define IRCMANAGER_H

#define TWITCH_MAX_MESSAGELENGTH 500

#include "account.h"
#include "messages/message.h"

#include <IrcMessage>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QString>

namespace chatterino {

class IrcManager
{
public:
    static void connect();
    static void disconnect();

    static void send(QString raw);

    static const QString defaultClientId;

    bool isTwitchBlockedUser(QString const &username);
    bool tryAddIgnoredUser(QString const &username, QString &errorMessage);
    void addIgnoredUser(QString const &username);
    bool tryRemoveIgnoredUser(QString const &username, QString &errorMessage);
    void removeIgnoredUser(QString const &username);

    static Account *account;

    static QNetworkAccessManager &
    getAccessManager()
    {
        return accessManager;
    }

    static void joinChannel(const QString &channel);

    static void partChannel(const QString &channel);

private:
    IrcManager();

    static QMap<QString, bool> twitchBlockedUsers;
    static QMutex twitchBlockedUsersMutex;

    static QNetworkAccessManager accessManager;

    static void beginConnecting();

    static IrcConnection *connection;
    static QMutex connectionMutex;
    static long connectionGeneration;

    static void messageReceived(IrcMessage *message);
    static void privateMessageReceived(IrcPrivateMessage *message);
};
}

#endif  // IRCMANAGER_H
