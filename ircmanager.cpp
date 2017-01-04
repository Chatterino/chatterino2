#include "ircmanager.h"
#include "ircconnection.h"
#include "irccommand.h"
#include "future"
#include "QNetworkReply"
#include "asyncexec.h"
#include "qnetworkrequest.h"
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"

Account*                IrcManager::account                 = NULL;
IrcConnection*          IrcManager::connection              = NULL;
QMutex*                 IrcManager::connectionMutex         = new QMutex();
long                    IrcManager::connectionIteration     = 0;
const                   QString IrcManager::defaultClientId = "7ue61iz46fz11y3cugd0l3tawb4taal";
QNetworkAccessManager*  IrcManager::accessManager           = new QNetworkAccessManager();

QMap<QString, bool>*    IrcManager::twitchBlockedUsers      = new QMap<QString, bool>;
QMutex*                 IrcManager::twitchBlockedUsersMutex = new QMutex();

IrcManager::IrcManager()
{
//    account = Account::anon();
}

void IrcManager::connect()
{
    disconnect();

    async_exec(beginConnecting());
}

void IrcManager::beginConnecting()
{
    int iteration = ++connectionIteration;

    auto c = new IrcConnection();

    QObject::connect(c,
                     &IrcConnection::messageReceived,
                     &messageReceived);
    QObject::connect(c,
                     &IrcConnection::privateMessageReceived,
                     &privateMessageReceived);

    if (account->isAnon()) {
        // fetch ignored users
        QString username = account->username();
        QString oauthClient = account->oauthClient();
        QString oauthToken = account->oauthToken();

        {
        QString nextLink = "https://api.twitch.tv/kraken/users/" + username +
            "/blocks?limit=" + 100 +
            "&client_id=" + oauthClient;

        QNetworkRequest req(QUrl(nextLink + "&oauth_token=" + oauthToken));
        QNetworkReply *reply = accessManager->get(req);

        QObject::connect(reply, &QNetworkReply::finished, [=]{
            twitchBlockedUsersMutex->lock();
            twitchBlockedUsers->clear();
            twitchBlockedUsersMutex->unlock();

            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            //nextLink = root.value("_links").toObject().value("next").toString();

            auto blocks = root.value("blocks").toArray();

            twitchBlockedUsersMutex->lock();
            for (QJsonValue block : blocks) {
                QJsonObject user = block.toObject().value("user").toObject();
                // display_name
                twitchBlockedUsers->insert(user.value("name").toString().toLower(), true);
            }
            twitchBlockedUsersMutex->unlock();
        });
        }

        // fetch available twitch emtoes
        {
        QNetworkRequest req(QUrl("https://api.twitch.tv/kraken/users/" + username + "/emotes?oauth_token=" + oauthToken + "&client_id=" + oauthClient));
        QNetworkReply *reply = accessManager->get(req);

        QObject::connect(reply, &QNetworkReply::finished, [=]{
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            //nextLink = root.value("_links").toObject().value("next").toString();

            auto blocks = root.value("blocks").toArray();

            twitchBlockedUsersMutex->lock();
            for (QJsonValue block : blocks) {
                QJsonObject user = block.toObject().value("user").toObject();
                // display_name
                twitchBlockedUsers->insert(user.value("name").toString().toLower(), true);
            }
            twitchBlockedUsersMutex->unlock();
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

    connectionMutex->lock();
    if (iteration == connectionIteration) {
        delete connection;
        c->moveToThread(QCoreApplication::instance()->thread());
        connection = c;
    }
    else {
        delete c;
    }
    connectionMutex->unlock();
}

void IrcManager::disconnect()
{
    connectionMutex->lock();

    if (connection != NULL) {
        delete connection;
        connection = NULL;
    }

    connectionMutex->unlock();
}

void IrcManager::messageReceived(IrcMessage *message)
{
    qInfo(message->command().toStdString().c_str());

//    if (message->command() == "")
}

void IrcManager::privateMessageReceived(IrcPrivateMessage *message)
{
    qInfo(message->content().toStdString().c_str());
}

bool IrcManager::isTwitchBlockedUser(QString const &username)
{
    twitchBlockedUsersMutex->lock();

    auto iterator = twitchBlockedUsers->find(username);

    if (iterator == twitchBlockedUsers->end()) {
        twitchBlockedUsersMutex->unlock();
        return false;
    }

    twitchBlockedUsersMutex->unlock();
    return true;
}

bool IrcManager::tryAddIgnoredUser(QString const &username, QString& errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + account->username() +
             "/blocks/" + username +
             "?oauth_token=" + account->oauthToken() +
             "&client_id=" + account->oauthClient());

    QNetworkRequest request(url);
    auto reply = accessManager->put(request, QByteArray());
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError)
    {
        twitchBlockedUsersMutex->lock();
        twitchBlockedUsers->insert(username, true);
        twitchBlockedUsersMutex->unlock();

        delete reply;
        return true;
    }

    errorMessage = "Error while ignoring user \"" + username + "\": " + reply->errorString();
    return false;
}

void IrcManager::addIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (tryAddIgnoredUser(username, errorMessage)) {
#warning "xD"
    }
}

bool IrcManager::tryRemoveIgnoredUser(QString const &username, QString& errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + account->username() +
             "/blocks/" + username +
             "?oauth_token=" + account->oauthToken() +
             "&client_id=" + account->oauthClient());

    QNetworkRequest request(url);
    auto reply = accessManager->deleteResource(request);
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError)
    {
        twitchBlockedUsersMutex->lock();
        twitchBlockedUsers->remove(username);
        twitchBlockedUsersMutex->unlock();

        delete reply;
        return true;
    }

    errorMessage = "Error while unignoring user \"" + username + "\": " + reply->errorString();
    return false;
}

void IrcManager::removeIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (tryRemoveIgnoredUser(username, errorMessage)) {
#warning "xD"
    }
}
