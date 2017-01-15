#include "ircmanager.h"
#include "QJsonArray"
#include "QJsonDocument"
#include "QJsonObject"
#include "QNetworkReply"
#include "asyncexec.h"
#include "channel.h"
#include "future"
#include "irccommand.h"
#include "ircconnection.h"
#include "qnetworkrequest.h"

Account *IrcManager::account = const_cast<Account *>(Account::anon());
IrcConnection *IrcManager::connection = NULL;
QMutex IrcManager::connectionMutex;
long IrcManager::connectionIteration = 0;
const QString IrcManager::defaultClientId = "7ue61iz46fz11y3cugd0l3tawb4taal";
QNetworkAccessManager IrcManager::m_accessManager;

QMap<QString, bool> IrcManager::twitchBlockedUsers;
QMutex IrcManager::twitchBlockedUsersMutex;

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
    int iteration = ++connectionIteration;

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
                twitchBlockedUsersMutex.lock();
                twitchBlockedUsers.clear();
                twitchBlockedUsersMutex.unlock();

                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                twitchBlockedUsersMutex.unlock();
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

                twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                twitchBlockedUsersMutex.unlock();
            });
        }
    }

    c->setHost("irc.chat.twitch.tv");
    c->setPort(6667);

    c->setUserName("justinfan123");
    c->setNickName("justinfan123");
    c->setRealName("justinfan123");
    c->sendRaw("JOIN #fourtf");

    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    c->open();

    connectionMutex.lock();
    if (iteration == connectionIteration) {
        delete connection;
        c->moveToThread(QCoreApplication::instance()->thread());
        connection = c;
    } else {
        delete c;
    }
    connectionMutex.unlock();
}

void
IrcManager::disconnect()
{
    connectionMutex.lock();

    if (connection != NULL) {
        delete connection;
        connection = NULL;
    }

    connectionMutex.unlock();
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
    qInfo(message->content().toStdString().c_str());

    qInfo(message->target().toStdString().c_str());
    auto c = Channel::getChannel(message->target().mid(1));

    if (c != NULL) {
        c->addMessage(std::shared_ptr<Message>(new Message(*message, *c)));
    }
}

bool
IrcManager::isTwitchBlockedUser(QString const &username)
{
    twitchBlockedUsersMutex.lock();

    auto iterator = twitchBlockedUsers.find(username);

    if (iterator == twitchBlockedUsers.end()) {
        twitchBlockedUsersMutex.unlock();
        return false;
    }

    twitchBlockedUsersMutex.unlock();
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
        twitchBlockedUsersMutex.lock();
        twitchBlockedUsers.insert(username, true);
        twitchBlockedUsersMutex.unlock();

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
#pragma message WARN("Implement IrcManager::addIgnoredUser")
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
        twitchBlockedUsersMutex.lock();
        twitchBlockedUsers.remove(username);
        twitchBlockedUsersMutex.unlock();

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
#pragma message WARN("TODO: Implement IrcManager::removeIgnoredUser")
    }
}
