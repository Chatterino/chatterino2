#include "ircmanager.h"
#include "asyncexec.h"
#include "channel.h"
#include "channelmanager.h"
#include "messages/messageparseargs.h"
#include "twitch/twitchmessagebuilder.h"
#include "twitch/twitchparsemessage.h"
#include "twitch/twitchuser.h"
#include "usermanager.h"

#include <irccommand.h>
#include <ircconnection.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <future>

using namespace  chatterino::messages;

namespace  chatterino {

IrcManager IrcManager::instance;

const QString IrcManager::defaultClientId("7ue61iz46fz11y3cugd0l3tawb4taal");

IrcManager::IrcManager()
    : _account(AccountManager::getInstance().getTwitchAnon())
    , _connection()
    , _connectionMutex()
    , _connectionGeneration(0)
    , _twitchBlockedUsers()
    , _twitchBlockedUsersMutex()
    , _accessManager()
{
}

const twitch::TwitchUser &IrcManager::getUser() const
{
    return _account;
}

void IrcManager::setUser(const twitch::TwitchUser &account)
{
    _account = account;
}

void IrcManager::connect()
{
    disconnect();

    async_exec([this] { beginConnecting(); });
}

void IrcManager::beginConnecting()
{
    int generation = ++IrcManager::_connectionGeneration;

    Communi::IrcConnection *c = new Communi::IrcConnection;

    QObject::connect(c, &Communi::IrcConnection::messageReceived, this,
                     &IrcManager::messageReceived);
    QObject::connect(c, &Communi::IrcConnection::privateMessageReceived, this,
                     &IrcManager::privateMessageReceived);

    if (_account.isAnon()) {
        // fetch ignored users
        QString username = _account.getUserName();
        QString oauthClient = _account.getOAuthClient();
        QString oauthToken = _account.getOAuthToken();

        {
            QString nextLink = "https://api.twitch.tv/kraken/users/" + username +
                               "/blocks?limit=" + 100 + "&client_id=" + oauthClient;

            QNetworkAccessManager *manager = new QNetworkAccessManager();
            QNetworkRequest req(QUrl(nextLink + "&oauth_token=" + oauthToken));
            QNetworkReply *reply = manager->get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                _twitchBlockedUsersMutex.lock();
                _twitchBlockedUsers.clear();
                _twitchBlockedUsersMutex.unlock();

                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                _twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user = block.toObject().value("user").toObject();
                    // display_name
                    _twitchBlockedUsers.insert(user.value("name").toString().toLower(), true);
                }
                _twitchBlockedUsersMutex.unlock();

                manager->deleteLater();
            });
        }

        // fetch available twitch emtoes
        {
            QNetworkRequest req(QUrl("https://api.twitch.tv/kraken/users/" + username +
                                     "/emotes?oauth_token=" + oauthToken +
                                     "&client_id=" + oauthClient));
            QNetworkReply *reply = _accessManager.get(req);

            QObject::connect(reply, &QNetworkReply::finished, [=] {
                QByteArray data = reply->readAll();
                QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
                QJsonObject root = jsonDoc.object();

                // nextLink =
                // root.value("_links").toObject().value("next").toString();

                auto blocks = root.value("blocks").toArray();

                _twitchBlockedUsersMutex.lock();
                for (QJsonValue block : blocks) {
                    QJsonObject user = block.toObject().value("user").toObject();
                    // display_name
                    _twitchBlockedUsers.insert(user.value("name").toString().toLower(), true);
                }
                _twitchBlockedUsersMutex.unlock();
            });
        }
    }

    c->setHost("irc.chat.twitch.tv");
    c->setPort(6667);

    c->setUserName("justinfan123");
    c->setNickName("justinfan123");
    c->setRealName("justinfan123");

    c->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    c->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    QMutexLocker locker(&_connectionMutex);

    if (generation == _connectionGeneration) {
        c->moveToThread(QCoreApplication::instance()->thread());
        _connection = std::shared_ptr<Communi::IrcConnection>(c);

        for (auto &channel : ChannelManager::getInstance().getItems()) {
            c->sendRaw("JOIN #" + channel->getName());
        }

        c->open();
    } else {
        delete c;
    }
}

void IrcManager::disconnect()
{
    _connectionMutex.lock();

    auto c = _connection;
    if (_connection.get() != NULL) {
        _connection = std::shared_ptr<Communi::IrcConnection>();
    }

    _connectionMutex.unlock();
}

void IrcManager::send(QString raw)
{
    _connectionMutex.lock();

    _connection->sendRaw(raw);

    _connectionMutex.unlock();
}

void IrcManager::sendJoin(const QString &channel)
{
    _connectionMutex.lock();

    if (_connection.get() != NULL) {
        _connection.get()->sendRaw("JOIN #" + channel);
    }

    _connectionMutex.unlock();
}

void IrcManager::partChannel(const QString &channel)
{
    _connectionMutex.lock();

    if (_connection.get() != NULL) {
        _connection.get()->sendRaw("PART #" + channel);
    }

    _connectionMutex.unlock();
}

void IrcManager::messageReceived(Communi::IrcMessage *message)
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

void IrcManager::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
    auto c = ChannelManager::getInstance().getChannel(message->target().mid(1));

    if (c != NULL) {
        messages::MessageParseArgs args;

        c->addMessage(twitch::TwitchMessageBuilder::parse(message, c.get(), args));
    }
}

bool IrcManager::isTwitchBlockedUser(QString const &username)
{
    QMutexLocker locker(&_twitchBlockedUsersMutex);

    auto iterator = _twitchBlockedUsers.find(username);

    return iterator != _twitchBlockedUsers.end();
}

bool IrcManager::tryAddIgnoredUser(QString const &username, QString &errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + _account.getUserName() + "/blocks/" +
             username + "?oauth_token=" + _account.getOAuthToken() +
             "&client_id=" + _account.getOAuthClient());

    QNetworkRequest request(url);
    auto reply = _accessManager.put(request, QByteArray());
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        _twitchBlockedUsersMutex.lock();
        _twitchBlockedUsers.insert(username, true);
        _twitchBlockedUsersMutex.unlock();

        return true;
    }

    reply->deleteLater();

    errorMessage = "Error while ignoring user \"" + username + "\": " + reply->errorString();
    return false;
}

void IrcManager::addIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryAddIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::addIgnoredUser
    }
}

bool IrcManager::tryRemoveIgnoredUser(QString const &username, QString &errorMessage)
{
    QUrl url("https://api.twitch.tv/kraken/users/" + _account.getUserName() + "/blocks/" +
             username + "?oauth_token=" + _account.getOAuthToken() +
             "&client_id=" + _account.getOAuthClient());

    QNetworkRequest request(url);
    auto reply = _accessManager.deleteResource(request);
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        _twitchBlockedUsersMutex.lock();
        _twitchBlockedUsers.remove(username);
        _twitchBlockedUsersMutex.unlock();

        return true;
    }

    reply->deleteLater();

    errorMessage = "Error while unignoring user \"" + username + "\": " + reply->errorString();
    return false;
}

void IrcManager::removeIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryRemoveIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::removeIgnoredUser
    }
}

QNetworkAccessManager &IrcManager::getAccessManager()
{
    return _accessManager;
}
}
