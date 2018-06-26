//#include "singletons/IrcManager.hpp"
//#include "Channel.hpp"
//#include "debug/Log.hpp"
//#include "messages/MessageParseArgs.hpp"
//#include "providers/twitch/TwitchAccount.hpp"
//#include "providers/twitch/TwitchMessageBuilder.hpp"
//#include "singletons/accountmanager.hpp"
//#include "singletons/channelmanager.hpp"
//#include "singletons/EmoteManager.hpp"
//#include "singletons/ResourceManager.hpp"
//#include "singletons/SettingsManager.hpp"
//#include "singletons/WindowManager.hpp"
//#include "util/PostToThread.hpp"
//#include "util/UrlFetch.hpp"

//#include <irccommand.h>
//#include <QJsonArray>
//#include <QJsonDocument>
//#include <QJsonObject>
//#include <QNetworkReply>
//#include <QNetworkRequest>

//#include <future>

//using namespace chatterino::messages;

// void IrcManager::refreshIgnoredUsers(const QString &username, const QString &oauthClient,
//                                     const QString &oauthToken)
//{
//    QString nextLink = "https://api.twitch.tv/kraken/users/" + username + "/blocks?limit=" + 100 +
//                       "&client_id=" + oauthClient;
//
//    QNetworkAccessManager *manager = new QNetworkAccessManager();
//    QNetworkRequest req(QUrl(nextLink + "&oauth_token=" + oauthToken));
//    QNetworkReply *reply = manager->get(req);
//
//    QObject::connect(reply, &QNetworkReply::finished, [=] {
//        this->twitchBlockedUsersMutex.lock();
//        this->twitchBlockedUsers.clear();
//        this->twitchBlockedUsersMutex.unlock();
//
//        QByteArray data = reply->readAll();
//        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
//        QJsonObject root = jsonDoc.object();
//
//        // nextLink =
//        // root.value("this->links").toObject().value("next").toString();
//
//        auto blocks = root.value("blocks").toArray();
//
//        this->twitchBlockedUsersMutex.lock();
//        for (QJsonValue block : blocks) {
//            QJsonObject user = block.toObject().value("user").toObject();
//            // displaythis->name
//            this->twitchBlockedUsers.insert(user.value("name").toString().toLower(), true);
//        }
//        this->twitchBlockedUsersMutex.unlock();
//
//        manager->deleteLater();
//    });
//}
//
//// XXX: This does not fit in IrcManager
// bool IrcManager::isTwitchUserBlocked(QString const &username)
//{
//    QMutexLocker locker(&this->twitchBlockedUsersMutex);
//
//    auto iterator = this->twitchBlockedUsers.find(username);
//
//    return iterator != this->twitchBlockedUsers.end();
//}
//
//// XXX: This does not fit in IrcManager
// bool IrcManager::tryAddIgnoredUser(QString const &username, QString &errorMessage)
//{
//    assert(this->account);
//
//    QUrl url("https://api.twitch.tv/kraken/users/" + this->account->getUserName() + "/blocks/" +
//             username + "?oauth_token=" + this->account->getOAuthToken() +
//             "&client_id=" + this->account->getOAuthClient());
//
//    QNetworkRequest request(url);
//    auto reply = this->networkAccessManager.put(request, QByteArray());
//    reply->waitForReadyRead(10000);
//
//    if (reply->error() == QNetworkReply::NoError) {
//        this->twitchBlockedUsersMutex.lock();
//        this->twitchBlockedUsers.insert(username, true);
//        this->twitchBlockedUsersMutex.unlock();
//
//        return true;
//    }
//
//    reply->deleteLater();
//
//    errorMessage = "Error while ignoring user \"" + username + "\": " + reply->errorString();
//
//    return false;
//}
//
//// XXX: This does not fit in IrcManager
// void IrcManager::addIgnoredUser(QString const &username)
//{
//    QString errorMessage;
//    if (!tryAddIgnoredUser(username, errorMessage)) {
//        // TODO: Implement IrcManager::addIgnoredUser
//    }
//}
//
//// XXX: This does not fit in IrcManager
// bool IrcManager::tryRemoveIgnoredUser(QString const &username, QString &errorMessage)
//{
//    assert(this->account);
//
//    QUrl url("https://api.twitch.tv/kraken/users/" + this->account->getUserName() + "/blocks/" +
//             username + "?oauth_token=" + this->account->getOAuthToken() +
//             "&client_id=" + this->account->getOAuthClient());
//
//    QNetworkRequest request(url);
//    auto reply = this->networkAccessManager.deleteResource(request);
//    reply->waitForReadyRead(10000);
//
//    if (reply->error() == QNetworkReply::NoError) {
//        this->twitchBlockedUsersMutex.lock();
//        this->twitchBlockedUsers.remove(username);
//        this->twitchBlockedUsersMutex.unlock();
//
//        return true;
//    }
//
//    reply->deleteLater();
//
//    errorMessage = "Error while unignoring user \"" + username + "\": " + reply->errorString();
//
//    return false;
//}
//
//// XXX: This does not fit in IrcManager
// void IrcManager::removeIgnoredUser(QString const &username)
//{
//    QString errorMessage;
//    if (!tryRemoveIgnoredUser(username, errorMessage)) {
//        // TODO: Implement IrcManager::removeIgnoredUser
//    }
//}
