#include "singletons/ircmanager.hpp"
#include "channel.hpp"
#include "debug/log.hpp"
#include "messages/messageparseargs.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/helper/ircmessagehandler.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/windowmanager.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "twitch/twitchuser.hpp"
#include "util/posttothread.hpp"
#include "util/urlfetch.hpp"

#include <irccommand.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <future>

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

IrcManager::IrcManager(ChannelManager &_channelManager, ResourceManager &_resources,
                       AccountManager &_accountManager)
    : channelManager(_channelManager)
    , resources(_resources)
    , accountManager(_accountManager)
{
    this->account = accountManager.Twitch.getCurrent();
    accountManager.Twitch.userChanged.connect([this]() {
        this->setUser(accountManager.Twitch.getCurrent());

        debug::Log("[IrcManager] Reconnecting to Twitch IRC as new user {}",
                   this->account->getUserName());

        postToThread([this] { this->connect(); });
    });

    // Initialize the connections
    this->writeConnection.reset(new Communi::IrcConnection);
    this->writeConnection->moveToThread(QCoreApplication::instance()->thread());

    QObject::connect(this->writeConnection.get(), &Communi::IrcConnection::messageReceived, this,
                     &IrcManager::writeConnectionMessageReceived);

    this->readConnection.reset(new Communi::IrcConnection);
    this->readConnection->moveToThread(QCoreApplication::instance()->thread());

    // Listen to read connection message signals
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::messageReceived, this,
                     &IrcManager::messageReceived);
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::privateMessageReceived,
                     this, &IrcManager::privateMessageReceived);

    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::connected, this,
                     &IrcManager::onConnected);
    QObject::connect(this->readConnection.get(), &Communi::IrcConnection::disconnected, this,
                     &IrcManager::onDisconnected);

    // join and part chats on event
    ChannelManager::getInstance().ircJoin.connect(
        [this](const QString &name) { this->joinChannel(name); });
    ChannelManager::getInstance().ircPart.connect(
        [this](const QString &name) { this->partChannel(name); });
}

IrcManager &IrcManager::getInstance()
{
    static IrcManager instance(ChannelManager::getInstance(),
                               singletons::ResourceManager::getInstance(),
                               AccountManager::getInstance());
    return instance;
}

void IrcManager::setUser(std::shared_ptr<twitch::TwitchUser> newAccount)
{
    this->account = newAccount;
}

void IrcManager::connect()
{
    this->disconnect();

    this->initializeConnection(this->writeConnection, false);
    this->initializeConnection(this->readConnection, true);

    // XXX(pajlada): Disabled the async_exec for now, because if we happen to run the
    // `beginConnecting` function in a different thread than last time, we won't be able to connect
    // because we can't clean up the previous connection properly
    // async_exec([this] { beginConnecting(); });
    this->beginConnecting();
}

void IrcManager::initializeConnection(const std::unique_ptr<Communi::IrcConnection> &connection,
                                      bool isReadConnection)
{
    assert(this->account);

    QString username = this->account->getUserName();
    QString oauthClient = this->account->getOAuthClient();
    QString oauthToken = this->account->getOAuthToken();
    if (!oauthToken.startsWith("oauth:")) {
        oauthToken.prepend("oauth:");
    }

    connection->setUserName(username);
    connection->setNickName(username);
    connection->setRealName(username);

    if (!this->account->isAnon()) {
        connection->setPassword(oauthToken);

        this->refreshIgnoredUsers(username, oauthClient, oauthToken);
    }

    if (isReadConnection) {
        connection->sendCommand(
            Communi::IrcCommand::createCapability("REQ", "twitch.tv/membership"));
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));
    } else {
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/tags"));

        connection->sendCommand(
            Communi::IrcCommand::createCapability("REQ", "twitch.tv/membership"));
        connection->sendCommand(Communi::IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    }

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);
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
        this->twitchBlockedUsersMutex.lock();
        this->twitchBlockedUsers.clear();
        this->twitchBlockedUsersMutex.unlock();

        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
        QJsonObject root = jsonDoc.object();

        // nextLink =
        // root.value("this->links").toObject().value("next").toString();

        auto blocks = root.value("blocks").toArray();

        this->twitchBlockedUsersMutex.lock();
        for (QJsonValue block : blocks) {
            QJsonObject user = block.toObject().value("user").toObject();
            // displaythis->name
            this->twitchBlockedUsers.insert(user.value("name").toString().toLower(), true);
        }
        this->twitchBlockedUsersMutex.unlock();

        manager->deleteLater();
    });
}

void IrcManager::beginConnecting()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex);

    for (auto &channel : this->channelManager.getItems()) {
        this->writeConnection->sendRaw("JOIN #" + channel->name);
        this->readConnection->sendRaw("JOIN #" + channel->name);
    }

    this->writeConnection->open();
    this->readConnection->open();

    this->connected();
}

void IrcManager::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex);

    this->readConnection->close();
    this->writeConnection->close();
}

void IrcManager::sendMessage(const QString &channelName, QString message)
{
    QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        return;
    }

    this->connectionMutex.lock();

    if (this->writeConnection) {
        this->writeConnection->sendRaw("PRIVMSG #" + channelName + " :" + trimmedMessage);
    }

    this->connectionMutex.unlock();
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
    auto c = this->channelManager.getTwitchChannel(message->target().mid(1));

    if (!c) {
        return;
    }

    auto xd = message->content();
    auto xd2 = message->toData();

    debug::Log("HEHE: {}", xd2.toStdString());

    messages::MessageParseArgs args;

    twitch::TwitchMessageBuilder builder(c.get(), message, args);

    if (!builder.isIgnored()) {
        messages::MessagePtr _message = builder.build();
        if (_message->hasFlags(messages::Message::Highlighted)) {
            singletons::ChannelManager::getInstance().mentionsChannel->addMessage(_message);
        }

        c->addMessage(_message);
    }
}

void IrcManager::messageReceived(Communi::IrcMessage *message)
{
    if (message->type() == Communi::IrcMessage::Type::Private) {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    if (command == "ROOMSTATE") {
        helper::IrcMessageHandler::getInstance().handleRoomStateMessage(message);
    } else if (command == "CLEARCHAT") {
        helper::IrcMessageHandler::getInstance().handleClearChatMessage(message);
    } else if (command == "USERSTATE") {
        helper::IrcMessageHandler::getInstance().handleUserStateMessage(message);
    } else if (command == "WHISPER") {
        helper::IrcMessageHandler::getInstance().handleWhisperMessage(message);
    } else if (command == "USERNOTICE") {
        helper::IrcMessageHandler::getInstance().handleUserNoticeMessage(message);
    } else if (command == "MODE") {
        helper::IrcMessageHandler::getInstance().handleModeMessage(message);
    } else if (command == "NOTICE") {
        helper::IrcMessageHandler::getInstance().handleNoticeMessage(
            static_cast<Communi::IrcNoticeMessage *>(message));
    }
}

void IrcManager::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
    switch (message->type()) {
        case Communi::IrcMessage::Type::Notice: {
            helper::IrcMessageHandler::getInstance().handleWriteConnectionNoticeMessage(
                static_cast<Communi::IrcNoticeMessage *>(message));
        } break;
    }
}

// XXX: This does not fit in IrcManager
bool IrcManager::isTwitchUserBlocked(QString const &username)
{
    QMutexLocker locker(&this->twitchBlockedUsersMutex);

    auto iterator = this->twitchBlockedUsers.find(username);

    return iterator != this->twitchBlockedUsers.end();
}

// XXX: This does not fit in IrcManager
bool IrcManager::tryAddIgnoredUser(QString const &username, QString &errorMessage)
{
    assert(this->account);

    QUrl url("https://api.twitch.tv/kraken/users/" + this->account->getUserName() + "/blocks/" +
             username + "?oauth_token=" + this->account->getOAuthToken() +
             "&client_id=" + this->account->getOAuthClient());

    QNetworkRequest request(url);
    auto reply = this->networkAccessManager.put(request, QByteArray());
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        this->twitchBlockedUsersMutex.lock();
        this->twitchBlockedUsers.insert(username, true);
        this->twitchBlockedUsersMutex.unlock();

        return true;
    }

    reply->deleteLater();

    errorMessage = "Error while ignoring user \"" + username + "\": " + reply->errorString();

    return false;
}

// XXX: This does not fit in IrcManager
void IrcManager::addIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryAddIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::addIgnoredUser
    }
}

// XXX: This does not fit in IrcManager
bool IrcManager::tryRemoveIgnoredUser(QString const &username, QString &errorMessage)
{
    assert(this->account);

    QUrl url("https://api.twitch.tv/kraken/users/" + this->account->getUserName() + "/blocks/" +
             username + "?oauth_token=" + this->account->getOAuthToken() +
             "&client_id=" + this->account->getOAuthClient());

    QNetworkRequest request(url);
    auto reply = this->networkAccessManager.deleteResource(request);
    reply->waitForReadyRead(10000);

    if (reply->error() == QNetworkReply::NoError) {
        this->twitchBlockedUsersMutex.lock();
        this->twitchBlockedUsers.remove(username);
        this->twitchBlockedUsersMutex.unlock();

        return true;
    }

    reply->deleteLater();

    errorMessage = "Error while unignoring user \"" + username + "\": " + reply->errorString();

    return false;
}

// XXX: This does not fit in IrcManager
void IrcManager::removeIgnoredUser(QString const &username)
{
    QString errorMessage;
    if (!tryRemoveIgnoredUser(username, errorMessage)) {
        // TODO: Implement IrcManager::removeIgnoredUser
    }
}

void IrcManager::onConnected()
{
    MessagePtr msg = Message::createSystemMessage("connected to chat");
    MessagePtr remsg = Message::createSystemMessage("reconnected to chat");

    this->channelManager.doOnAll([msg, remsg](ChannelPtr channel) {
        assert(channel);

        LimitedQueueSnapshot<MessagePtr> snapshot = channel->getMessageSnapshot();

        if (snapshot.getLength() > 0 &&
            snapshot[snapshot.getLength() - 1]->hasFlags(Message::DisconnectedMessage))  //
        {
            channel->replaceMessage(snapshot[snapshot.getLength() - 1], remsg);
            return;
        }

        channel->addMessage(msg);
    });
}

void IrcManager::onDisconnected()
{
    MessagePtr msg = Message::createSystemMessage("disconnected from chat");
    msg->addFlags(Message::DisconnectedMessage);

    this->channelManager.doOnAll([msg](ChannelPtr channel) {
        assert(channel);
        channel->addMessage(msg);
    });
}

Communi::IrcConnection *IrcManager::getReadConnection()
{
    return this->readConnection.get();
}

void IrcManager::addFakeMessage(const QString &data)
{
    auto fakeMessage = Communi::IrcMessage::fromData(data.toUtf8(), this->readConnection.get());

    this->privateMessageReceived(qobject_cast<Communi::IrcPrivateMessage *>(fakeMessage));
}

}  // namespace singletons
}  // namespace chatterino
