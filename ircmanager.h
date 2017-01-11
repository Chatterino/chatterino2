#ifndef IRCMANAGER_H
#define IRCMANAGER_H

#define TWITCH_MAX_MESSAGELENGTH 500

#include "IrcMessage"
#include "QMap"
#include "QMutex"
#include "QString"
#include "account.h"
#include "message.h"
#include "qnetworkaccessmanager.h"

class IrcManager
{
public:
    static void connect();
    static void disconnect();

    static const QString defaultClientId;

    bool isTwitchBlockedUser(QString const &username);
    bool tryAddIgnoredUser(QString const &username, QString &errorMessage);
    void addIgnoredUser(QString const &username);
    bool tryRemoveIgnoredUser(QString const &username, QString &errorMessage);
    void removeIgnoredUser(QString const &username);

    static Account *account;

    static QNetworkAccessManager &
    accessManager()
    {
        return m_accessManager;
    }

private:
    IrcManager();

    static QMap<QString, bool> twitchBlockedUsers;
    static QMutex twitchBlockedUsersMutex;

    static QNetworkAccessManager m_accessManager;

    static void beginConnecting();

    static IrcConnection *connection;
    static QMutex connectionMutex;
    static long connectionIteration;

    static void messageReceived(IrcMessage *message);
    static void privateMessageReceived(IrcPrivateMessage *message);
};

#endif  // IRCMANAGER_H
