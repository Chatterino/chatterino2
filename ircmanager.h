#ifndef IRCMANAGER_H
#define IRCMANAGER_H

#define TWITCH_MAX_MESSAGELENGTH 500

#include "messages/message.h"
#include "twitch/twitchuser.h"

#include <IrcMessage>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QString>

#include <memory>

namespace chatterino {

class IrcManager : public QObject
{
    Q_OBJECT

public:
    static IrcManager &getInstance()
    {
        return instance;
    }

    static const QString defaultClientId;

    void connect();
    void disconnect();

    void send(QString raw);

    bool isTwitchBlockedUser(QString const &username);
    bool tryAddIgnoredUser(QString const &username, QString &errorMessage);
    void addIgnoredUser(QString const &username);
    bool tryRemoveIgnoredUser(QString const &username, QString &errorMessage);
    void removeIgnoredUser(QString const &username);

    QNetworkAccessManager &getAccessManager();

    void sendJoin(const QString &channel);
    void sendMessage(const QString &channelName, const QString &message);

    void partChannel(const QString &channel);

    const twitch::TwitchUser &getUser() const;
    void setUser(const twitch::TwitchUser &account);

private:
    static IrcManager instance;
    IrcManager();

    // variables
    twitch::TwitchUser _account;

    std::shared_ptr<Communi::IrcConnection> _connection;
    QMutex _connectionMutex;
    long _connectionGeneration;

    QMap<QString, bool> _twitchBlockedUsers;
    QMutex _twitchBlockedUsersMutex;

    QNetworkAccessManager _accessManager;

    // methods
    void beginConnecting();

    void messageReceived(Communi::IrcMessage *message);
    void privateMessageReceived(Communi::IrcPrivateMessage *message);
};
}  // namespace chatterino

#endif  // IRCMANAGER_H
