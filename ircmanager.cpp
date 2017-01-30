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

namespace chatterino {

Account *IrcManager::account = nullptr;
std::shared_ptr<IrcConnection> IrcManager::connection;
QMutex IrcManager::connectionMutex;
long IrcManager::connectionGeneration = 0;
const QString IrcManager::defaultClientId = "7ue61iz46fz11y3cugd0l3tawb4taal";
QNetworkAccessManager IrcManager::accessManager;

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
    IrcManager::account = const_cast<Account *>(Account::getAnon());

    int generation = ++IrcManager::connectionGeneration;

    auto c = new IrcConnection();

    QObject::connect(c, &IrcConnection::messageReceived, &messageReceived);
    QObject::connect(c, &IrcConnection::privateMessageReceived,
                     &privateMessageReceived);

    if (account->isAnon()) {
        // fetch ignored users
        QString username = account->getUsername();
        QString oauthClient = account->getOauthClient();
        QString oauthToken = account->getOauthToken();

        {
            QString nextLink = "https://api.twitch.tv/kraken/users/" +
                               username + "/blocks?limit=" + 100 +
                               "&client_id=" + oauthClient;

            QNetworkAccessManager *manager = new QNetworkAccessManager();
            QNetworkRequest req(QUrl(nextLink + "&oauth_token=" + oauthToken));
            QNetworkReply *reply = manager->get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                IrcManager::twitchBlockedUsersMutex.lock();
                IrcManager::twitchBlockedUsers.clear();
                IrcManager::twitchBlockedUsersMutex.unlock();

                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                IrcManager::twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    IrcManager::twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                IrcManager::twitchBlockedUsersMutex.unlock();

                manager->deleteLater();
            });
        }

        // fetch available twitch emtoes
        {
            QNetworkRequest req(QUrl("https://api.twitch.tv/kraken/users/" +
                                     username + "/emotes?oauth_token=" +
                                     oauthToken + "&client_id=" + oauthClient));
            QNetworkReply *reply = IrcManager::accessManager.get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                IrcManager::twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user =
                        block.toObject().value("user").toObject();
                    // display_name
                    IrcManager::twitchBlockedUsers.insert(
                        user.value("name").toString().toLower(), true);
                }
                IrcManager::twitchBlockedUsersMutex.unlock();
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

    QMutexLocker locker(&IrcManager::connectionMutex);

    if (generation == IrcManager::connectionGeneration) {
        c->moveToThread(QCoreApplication::instance()->thread());
        IrcManager::connection = std::shared_ptr<IrcConnection>(c);

        auto channels = Channels::getItems();

        for (auto &channel : channels) {
            c->sendRaw("JOIN #" + channel.get()->getName());
        }

        c->open();
    } else {
        delete c;
    }
}

void
IrcManager::disconnect()
{
    IrcManager::connectionMutex.lock();

    auto c = IrcManager::connection;
    if (IrcManager::connection.get() != NULL) {
        IrcManager::connection = std::shared_ptr<IrcConnection>();
    }

    IrcManager::connectionMutex.unlock();
}

void
IrcManager::send(QString raw)
{
    IrcManager::connectionMutex.lock();
    IrcManager::connection->sendRaw(raw);
    IrcManager::connectionMutex.unlock();
}

void
IrcManager::sendJoin(const QString &channel)
{
    IrcManager::connectionMutex.lock();

    if (IrcManager::connection.get() != NULL) {
        IrcManager::connection.get()->sendRaw("JOIN #" + channel);
    }

    IrcManager::connectionMutex.unlock();
}

void
IrcManager::partChannel(const QString &channel)
{
    IrcManager::connectionMutex.lock();

    if (IrcManager::connection.get() != NULL) {
        IrcManager::connection.get()->sendRaw("PART #" + channel);
    }

    IrcManager::connectionMutex.unlock();
}

void
IrcManager::messageReceived(IrcMessage *message)
{
    //    qInfo(message->command().toStdString().c_str());

    const QString &command = message->command();

    //    if (command == "CLEARCHAT") {
    //        message->
    //    } else if (command == "ROOMSTATE") {
    //    } else if (command == "USERSTATE") {
    //    } else if (command == "WHISPER") {
    //    } else if (command == "USERNOTICE") {
    //    }
}

void
IrcManager::privateMessageReceived(IrcPrivateMessage *message)
{
    //    qInfo(message->content().toStdString().c_str());

    auto c = Channels::getChannel(message->target().mid(1));

    if (c != NULL) {
        c->addMessage(std::shared_ptr<messages::Message>(
            new messages::Message(*message, *c)));
    }
}

bool
IrcManager::isTwitchBlockedUser(QString const &username)
{
    IrcManager::twitchBlockedUsersMutex.lock();

    auto iterator = IrcManager::twitchBlockedUsers.find(username);

    if (iterator == IrcManager::twitchBlockedUsers.end()) {
        IrcManager::twitchBlockedUsersMutex.unlock();
        return false;
    }

    IrcManager::twitchBlockedUsersMutex.unlock();
    return true;
}

bool
IrcManager::tryAddIgnoredUser(QString const &username, QString &errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + account->getUsername() +
             "/blocks/" + username +
             "?oauth_token=" + account->getOauthToken() +
             "&client_id=" + account->getOauthClient());

    QNetworkRequest request(url);
    auto reply = IrcManager::accessManager.put(request, QByteArray());
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        IrcManager::twitchBlockedUsersMutex.lock();
        IrcManager::twitchBlockedUsers.insert(username, true);
        IrcManager::twitchBlockedUsersMutex.unlock();

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
    QUrl url("https://api.twitch.tv/kraken/users/" + account->getUsername() +
             "/blocks/" + username +
             "?oauth_token=" + account->getOauthToken() +
             "&client_id=" + account->getOauthClient());

    QNetworkRequest request(url);
    auto reply = IrcManager::accessManager.deleteResource(request);
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        IrcManager::twitchBlockedUsersMutex.lock();
        IrcManager::twitchBlockedUsers.remove(username);
        IrcManager::twitchBlockedUsersMutex.unlock();

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
}
