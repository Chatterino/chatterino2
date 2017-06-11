#pragma once

#define TWITCH_MAX_MESSAGELENGTH 500

#include "messages/message.hpp"
#include "twitch/twitchuser.hpp"

#include <IrcMessage>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QString>

#include <memory>
#include <mutex>

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

    bool isTwitchBlockedUser(QString const &username);
    bool tryAddIgnoredUser(QString const &username, QString &errorMessage);
    void addIgnoredUser(QString const &username);
    bool tryRemoveIgnoredUser(QString const &username, QString &errorMessage);
    void removeIgnoredUser(QString const &username);

    QNetworkAccessManager &getAccessManager();

    void sendMessage(const QString &channelName, const QString &message);

    void joinChannel(const QString &channelName);
    void partChannel(const QString &channelName);

    const twitch::TwitchUser &getUser() const;
    void setUser(const twitch::TwitchUser &account);

private:
    static IrcManager instance;
    IrcManager();

    // variables
    twitch::TwitchUser _account;

    std::shared_ptr<Communi::IrcConnection> writeConnection = nullptr;
    std::shared_ptr<Communi::IrcConnection> readConnection = nullptr;

    std::mutex connectionMutex;
    uint32_t connectionGeneration = 0;

    QMap<QString, bool> _twitchBlockedUsers;
    QMutex _twitchBlockedUsersMutex;

    QNetworkAccessManager _accessManager;

    // methods
    Communi::IrcConnection *createConnection(bool doRead);

    void refreshIgnoredUsers(const QString &username, const QString &oauthClient,
                             const QString &oauthToken);
    void refreshTwitchEmotes(const QString &username, const QString &oauthClient,
                             const QString &oauthToken);

    void beginConnecting();

    void messageReceived(Communi::IrcMessage *message);
    void privateMessageReceived(Communi::IrcPrivateMessage *message);
};

}  // namespace chatterino
