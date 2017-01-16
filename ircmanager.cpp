#include "ircmanager.h"
#include "asyncexec.h"
#include "channel.h"
#include "channels.h"

#include <irccommand.h>
#include <ircconnection.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <future>

Account *IrcManager::account = nullptr;
IrcConnection *IrcManager::m_connection = NULL;
QMutex IrcManager::m_connectionMutex;
long IrcManager::m_connectionIteration = 0;
const QString IrcManager::defaultClientId = "7ue61iz46fz11y3cugd0l3tawb4taal";
QNetworkAccessManager IrcManager::m_accessManager;

QMap<QString, bool> IrcManager::m_twitchBlockedUsers;
QMutex IrcManager::m_twitchBlockedUsersMutex;

IrcManager::IrcManager()
{
}

void
IrcManager::connect()
{
    disconnect();

    async_exec([] { beginConnecting(); });
}

void
IrcManager::beginConnecting()
{
    IrcManager::account = const_cast<Account *>(Account::anon());

    int iteration = ++m_connectionIteration;

    auto c = new IrcConnection();

    QObject::connect(c, &IrcConnection::messageReceived, &messageReceived);
    QObject::connect(c, &IrcConnection::privateMessageReceived,
                     &privateMessageReceived);

    if (account->isAnon()) {
        // fetch ignored users
        QString username = account->username();
        QString oauthClient = account->oauthClient();
        QString oauthToken = account->oauthToken();

        {
            QString nextLink = "https://api.twitch.tv/kraken/users/" +
                               username + "/blocks?limit=" + 100 +
                               "&client_id=" + oauthClient;

            QNetworkRequest req(QUrl(nextLink + "&oauth_token=" + oauthToken));
            QNetworkReply *reply = m_accessManager.get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                m_twitchBlockedUsersMutex.lock();
                m_twitchBlockedUsers.clear();
                m_twitchBlockedUsersMutex.unlock();

                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                m_twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    m_twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                m_twitchBlockedUsersMutex.unlock();
            });
        }

        // fetch available twitch emtoes
        {
            QNetworkRequest req(QUrl("https://api.twitch.tv/kraken/users/" +
                                     username + "/emotes?oauth_token=" +
                                     oauthToken + "&client_id=" + oauthClient));
            QNetworkReply *reply = m_accessManager.get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                m_twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    m_twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                m_twitchBlockedUsersMutex.unlock();
            });
        }
    }

    c->setHost("irc.chat.twitch.tv");
    c->setPort(6667);

    c->setUserName("justinfan123");
    c->setNickName("justinfan123");
    c->setRealName("justinfan123");

    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    c->open();

    m_connectionMutex.lock();
    if (iteration == m_connectionIteration) {
        delete m_connection;
        c->moveToThread(QCoreApplication::instance()->thread());
        m_connection = c;
    } else {
        delete c;
    }
    m_connectionMutex.unlock();
}

void
IrcManager::disconnect()
{
    m_connectionMutex.lock();

    if (m_connection != NULL) {
        delete m_connection;
        m_connection = NULL;
    }

    m_connectionMutex.unlock();
}

void
IrcManager::joinChannel(const QString &channel)
{
    m_connectionMutex.lock();
    if (m_connection != NULL) {
        m_connection->sendRaw("JOIN #" + channel);
    }
    m_connectionMutex.unlock();
}

void
IrcManager::partChannel(const QString &channel)
{
    m_connectionMutex.lock();
    if (m_connection != NULL) {
        m_connection->sendRaw("PART #" + channel);
    }
    m_connectionMutex.unlock();
}

void
IrcManager::messageReceived(IrcMessage *message)
{
    qInfo(message->command().toStdString().c_str());

    //    if (message->command() == "")
}

void
IrcManager::privateMessageReceived(IrcPrivateMessage *message)
{
    //    qInfo(message->content().toStdString().c_str());

    auto c = Channels::getChannel(message->target().mid(1));

    if (c != NULL) {
        c->addMessage(std::shared_ptr<Message>(new Message(*message, *c)));
    }
}

bool
IrcManager::isTwitchBlockedUser(QString const &username)
{
    m_twitchBlockedUsersMutex.lock();

    auto iterator = m_twitchBlockedUsers.find(username);

    if (iterator == m_twitchBlockedUsers.end()) {
        m_twitchBlockedUsersMutex.unlock();
        return false;
    }

    m_twitchBlockedUsersMutex.unlock();
    return true;
}

bool
IrcManager::tryAddIgnoredUser(QString const &username, QString &errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + account->username() +
             "/blocks/" + username + "?oauth_token=" + account->oauthToken() +
             "&client_id=" + account->oauthClient());

    QNetworkRequest request(url);
    auto reply = m_accessManager.put(request, QByteArray());
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        m_twitchBlockedUsersMutex.lock();
        m_twitchBlockedUsers.insert(username, true);
        m_twitchBlockedUsersMutex.unlock();

        delete reply;
        return true;
    }

    errorMessage = "Error while ignoring user \"" + username +
                   "\": " + reply->errorString();
    return false;
}

void
IrcManager::addIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryAddIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::addIgnoredUser
    }
}

bool
IrcManager::tryRemoveIgnoredUser(QString const &username, QString &errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + account->username() +
             "/blocks/" + username + "?oauth_token=" + account->oauthToken() +
             "&client_id=" + account->oauthClient());

    QNetworkRequest request(url);
    auto reply = m_accessManager.deleteResource(request);
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        m_twitchBlockedUsersMutex.lock();
        m_twitchBlockedUsers.remove(username);
        m_twitchBlockedUsersMutex.unlock();

        delete reply;
        return true;
    }

    errorMessage = "Error while unignoring user \"" + username +
                   "\": " + reply->errorString();
    return false;
}

void
IrcManager::removeIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryRemoveIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::removeIgnoredUser
    }
}
