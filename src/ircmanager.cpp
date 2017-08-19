#include "ircmanager.hpp"
#include "accountmanager.hpp"
#include "asyncexec.hpp"
#include "channel.hpp"
#include "channelmanager.hpp"
#include "emotemanager.hpp"
#include "messages/messageparseargs.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "twitch/twitchparsemessage.hpp"
#include "twitch/twitchuser.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <irccommand.h>
#include <ircconnection.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <future>

using namespace chatterino::messages;

namespace chatterino {

IrcManager::IrcManager(ChannelManager &_channelManager, Resources &_resources,
                       EmoteManager &_emoteManager, WindowManager &_windowManager)
    : channelManager(_channelManager)
    , resources(_resources)
    , emoteManager(_emoteManager)
    , windowManager(_windowManager)
    , _account(AccountManager::getInstance().getTwitchAnon())
    , currentUser("/accounts/current")
{
    this->currentUser.getValueChangedSignal().connect([](const auto &newUsername) {
        // TODO: Implement
        qDebug() << "Current user changed, fetch new credentials and reconnect";
    });
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

Communi::IrcConnection *IrcManager::createConnection(bool doRead)
{
    Communi::IrcConnection *connection = new Communi::IrcConnection;

    if (doRead) {
        QObject::connect(connection, &Communi::IrcConnection::messageReceived, this,
                         &IrcManager::messageReceived);
        QObject::connect(connection, &Communi::IrcConnection::privateMessageReceived, this,
                         &IrcManager::privateMessageReceived);
    }

    QString username = _account.getUserName();
    QString oauthClient = _account.getOAuthClient();
    QString oauthToken = _account.getOAuthToken();
    if (!oauthToken.startsWith("oauth:")) {
        oauthToken.prepend("oauth:");
    }

    connection->setUserName(username);
    connection->setNickName(username);
    connection->setRealName(username);

    if (!_account.isAnon()) {
        connection->setPassword(oauthToken);

        this->refreshIgnoredUsers(username, oauthClient, oauthToken);
    }

    if (doRead) {
        connection->sendCommand(
            Communi::IrcCommand::createCapability("REQ", "twitch.tv/membership"));
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));
    }

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);

    return connection;
}

void IrcManager::refreshIgnoredUsers(const QString &username, const QString &oauthClient,
                                     const QString &oauthToken)
{
    QString nextLink = "https://api.twitch.tv/kraken/users/" + username + "/blocks?limit=" + 100 +
                       "&client_id=" + oauthClient;

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

void IrcManager::beginConnecting()
{
    uint32_t generation = ++this->connectionGeneration;

    Communi::IrcConnection *_writeConnection = this->createConnection(false);
    Communi::IrcConnection *_readConnection = this->createConnection(true);

    std::lock_guard<std::mutex> locker(this->connectionMutex);

    if (generation == this->connectionGeneration) {
        this->writeConnection = std::shared_ptr<Communi::IrcConnection>(_writeConnection);
        this->readConnection = std::shared_ptr<Communi::IrcConnection>(_readConnection);

        this->writeConnection->moveToThread(QCoreApplication::instance()->thread());
        this->readConnection->moveToThread(QCoreApplication::instance()->thread());

        for (auto &channel : this->channelManager.getItems()) {
            this->writeConnection->sendRaw("JOIN #" + channel->name);
            this->readConnection->sendRaw("JOIN #" + channel->name);
        }

        this->writeConnection->open();
        this->readConnection->open();
    } else {
        delete _writeConnection;
        delete _readConnection;
    }
}

void IrcManager::disconnect()
{
    this->connectionMutex.lock();

    auto _readConnection = this->readConnection;
    auto _writeConnection = this->writeConnection;

    this->readConnection.reset();
    this->writeConnection.reset();

    this->connectionMutex.unlock();
}

void IrcManager::sendMessage(const QString &channelName, const QString &message)
{
    this->connectionMutex.lock();

    if (this->writeConnection) {
        this->writeConnection->sendRaw("PRIVMSG #" + channelName + " :" + message);
    }

    this->connectionMutex.unlock();

    // DEBUGGING
    /*
    Communi::IrcPrivateMessage msg(this->readConnection.get());

    QStringList params{"#pajlada", message};

    qDebug() << params;

    if (message == "COMIC SANS LOL") {
        FontManager::getInstance().currentFontFamily = "Comic Sans MS";
    } else if (message == "ARIAL LOL") {
        FontManager::getInstance().currentFontFamily = "Arial";
    } else if (message == "WINGDINGS LOL") {
        FontManager::getInstance().currentFontFamily = "Wingdings";
    }

    msg.setParameters(params);

    msg.setPrefix("pajlada!pajlada@pajlada");

    this->privateMessageReceived(&msg);
    */
}

void IrcManager::joinChannel(const QString &channelName)
{
    this->connectionMutex.lock();

    if (this->readConnection && this->writeConnection) {
        this->readConnection->sendRaw("JOIN #" + channelName);
        this->writeConnection->sendRaw("JOIN #" + channelName);
    }

    this->connectionMutex.unlock();
}

void IrcManager::partChannel(const QString &channelName)
{
    this->connectionMutex.lock();

    if (this->readConnection && this->writeConnection) {
        this->readConnection->sendRaw("PART #" + channelName);
        this->writeConnection->sendRaw("PART #" + channelName);
    }

    this->connectionMutex.unlock();
}

void IrcManager::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
    this->onPrivateMessage.invoke(message);
    auto c = this->channelManager.getChannel(message->target().mid(1));

    if (!c) {
        return;
    }

    messages::MessageParseArgs args;

    twitch::TwitchMessageBuilder builder(c.get(), this->resources, this->emoteManager,
                                         this->windowManager, message, args);

    c->addMessage(builder.parse());
}

void IrcManager::messageReceived(Communi::IrcMessage *message)
{
    if (message->type() == Communi::IrcMessage::Type::Private) {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    if (command == "ROOMSTATE") {
        this->handleRoomStateMessage(message);
    } else if (command == "CLEARCHAT") {
        this->handleClearChatMessage(message);
    } else if (command == "USERSTATE") {
        this->handleUserStateMessage(message);
    } else if (command == "WHISPER") {
        this->handleWhisperMessage(message);
    } else if (command == "USERNOTICE") {
        this->handleUserNoticeMessage(message);
    }
}

void IrcManager::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();

    auto iterator = tags.find("room-id");

    if (iterator != tags.end()) {
        std::string roomID = iterator.value().toString().toStdString();

        this->resources.loadChannelData(roomID);
    }
}

void IrcManager::handleClearChatMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcManager::handleUserStateMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcManager::handleWhisperMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcManager::handleUserNoticeMessage(Communi::IrcMessage *message)
{
    // do nothing
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
    auto reply = this->networkAccessManager.put(request, QByteArray());
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
    auto reply = this->networkAccessManager.deleteResource(request);
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

}  // namespace chatterino
