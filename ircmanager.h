#ifndef IRCMANAGER_H
#define IRCMANAGER_H

#define TWITCH_MAX_MESSAGELENGTH 500

#include "account.h"
#include "message.h"

#include <IrcMessage>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QString>

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

    static void joinChannel(const QString &channel);

    static void partChannel(const QString &channel);

private:
    IrcManager();

    static QMap<QString, bool> m_twitchBlockedUsers;
    static QMutex m_twitchBlockedUsersMutex;

    static QNetworkAccessManager m_accessManager;

    static void beginConnecting();

    static IrcConnection *m_connection;
    static QMutex m_connectionMutex;
    static long m_connectionIteration;

    static void messageReceived(IrcMessage *message);
    static void privateMessageReceived(IrcPrivateMessage *message);
};

#endif  // IRCMANAGER_H
