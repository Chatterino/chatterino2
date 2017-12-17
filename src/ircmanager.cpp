#include "ircmanager.hpp"
#include "accountmanager.hpp"
#include "asyncexec.hpp"
#include "channel.hpp"
#include "channelmanager.hpp"
#include "debug/log.hpp"
#include "emotemanager.hpp"
#include "messages/messageparseargs.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "twitch/twitchparsemessage.hpp"
#include "twitch/twitchuser.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <irccommand.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <future>

using namespace chatterino::messages;

namespace chatterino {

IrcManager::IrcManager(ChannelManager &_channelManager, Resources &_resources,
                       WindowManager &_windowManager)
    : channelManager(_channelManager)
    , resources(_resources)
    , windowManager(_windowManager)
{
    AccountManager::getInstance().Twitch.userChanged.connect([this]() {
        this->setUser(AccountManager::getInstance().Twitch.getCurrent());

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
}

void IrcManager::disconnect()
{
    std::lock_guard<std::mutex> locker(this->connectionMutex);

    this->readConnection->close();
    this->writeConnection->close();
}

void IrcManager::sendMessage(const QString &channelName, const QString &message)
{
    this->connectionMutex.lock();

    if (this->writeConnection) {
        this->writeConnection->sendRaw("PRIVMSG #" + channelName + " :" + message);
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

    messages::MessageParseArgs args;

    twitch::TwitchMessageBuilder builder(c.get(), this->resources, this->windowManager, message,
                                         args);

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
    } else if (command == "MODE") {
        this->handleModeMessage(message);
    } else if (command == "NOTICE") {
        this->handleNoticeMessage(static_cast<Communi::IrcNoticeMessage *>(message));
    }
}

void IrcManager::writeConnectionMessageReceived(Communi::IrcMessage *message)
{
    switch (message->type()) {
        case Communi::IrcMessage::Type::Notice: {
            this->handleWriteConnectionNoticeMessage(
                static_cast<Communi::IrcNoticeMessage *>(message));
        } break;
    }
}

void IrcManager::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();

    auto iterator = tags.find("room-id");

    if (iterator != tags.end()) {
        auto roomID = iterator.value().toString();

        auto channel = QString(message->toData()).split("#").at(1);
        channelManager.getTwitchChannel(channel)->setRoomID(roomID);

        this->resources.loadChannelData(roomID);
    }
}

void IrcManager::handleClearChatMessage(Communi::IrcMessage *message)
{
    assert(message->parameters().length() >= 1);

    auto rawChannelName = message->parameter(0);

    assert(rawChannelName.length() >= 2);

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log("[IrcManager:handleClearChatMessage] Channel {} not found in channel manager",
                   trimmedChannelName);
        return;
    }

    if (message->parameters().length() == 1) {
        std::shared_ptr<Message> msg(
            Message::createSystemMessage("Chat has been cleared by a moderator."));

        c->addMessage(msg);

        return;
    }

    assert(message->parameters().length() >= 2);

    QString username = message->parameter(1);
    QString durationInSeconds, reason;
    QVariant v = message->tag("ban-duration");
    if (v.isValid()) {
        durationInSeconds = v.toString();
    }

    v = message->tag("ban-reason");
    if (v.isValid()) {
        reason = v.toString();
    }

    std::shared_ptr<Message> msg(
        Message::createTimeoutMessage(username, durationInSeconds, reason));

    c->addMessage(msg);
}

void IrcManager::handleUserStateMessage(Communi::IrcMessage *message)
{
    // TODO: Implement
}

void IrcManager::handleWhisperMessage(Communi::IrcMessage *message)
{
    // TODO: Implement
}

void IrcManager::handleUserNoticeMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcManager::handleModeMessage(Communi::IrcMessage *message)
{
    auto channel = channelManager.getTwitchChannel(message->parameter(0).remove(0, 1));
    if (message->parameter(1) == "+o") {
        channel->modList.append(message->parameter(2));
    } else if (message->parameter(1) == "-o") {
        channel->modList.append(message->parameter(2));
    }
}

// XXX: This does not fit in IrcManager
bool IrcManager::isTwitchBlockedUser(QString const &username)
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

void IrcManager::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto rawChannelName = message->target();

    assert(rawChannelName.length() >= 2);

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager",
                   trimmedChannelName);
        return;
    }

    std::shared_ptr<Message> msg(Message::createSystemMessage(message->content()));

    c->addMessage(msg);
}

void IrcManager::handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto rawChannelName = message->target();

    assert(rawChannelName.length() >= 2);

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager",
                   trimmedChannelName);
        return;
    }

    QVariant v = message->tag("msg-id");
    if (!v.isValid()) {
        return;
    }
    QString msg_id = v.toString();

    static QList<QString> idsToSkip = {"timeout_success", "ban_success"};

    if (idsToSkip.contains(msg_id)) {
        // Already handled in the read-connection
        return;
    }

    std::shared_ptr<Message> msg(Message::createSystemMessage(message->content()));

    c->addMessage(msg);
}

void IrcManager::onConnected()
{
    std::shared_ptr<Message> msg(Message::createSystemMessage("connected to chat"));

    this->channelManager.doOnAll([msg](std::shared_ptr<twitch::TwitchChannel> channel) {
        assert(channel);
        channel->addMessage(msg);
    });
}

void IrcManager::onDisconnected()
{
    std::shared_ptr<Message> msg(Message::createSystemMessage("disconnected from chat"));

    this->channelManager.doOnAll([msg](std::shared_ptr<twitch::TwitchChannel> channel) {
        assert(channel);
        channel->addMessage(msg);
    });
}

}  // namespace chatterino
