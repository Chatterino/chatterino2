#pragma once

#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/Credentials.hpp"
#include "debug/Log.hpp"
#include "common/NetworkManager.hpp"
#include "common/NetworkRequest.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>
#include <QByteArray>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include <functional>

namespace chatterino {
namespace util {
namespace twitch {

static void get(QString url, const QObject *caller,
                std::function<void(const QJsonObject &)> successCallback)
{
    util::NetworkRequest req(url);
    req.setCaller(caller);
    req.setRawHeader("Client-ID", getDefaultClientID());
    req.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");

    req.getJSON([=](const QJsonObject &node) {
        successCallback(node);  //
    });
}

static void get2(QString url, const QObject *caller, bool useQuickLoadCache,
                 std::function<void(const rapidjson::Document &)> successCallback)
{
    util::NetworkRequest req(url);
    req.setCaller(caller);
    req.setRawHeader("Client-ID", getDefaultClientID());
    req.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    req.setUseQuickLoadCache(useQuickLoadCache);

    req.getJSON2([=](const rapidjson::Document &document) {
        successCallback(document);  //
    });
}

static void getAuthorized(QString url, const QString &clientID, const QString &oauthToken,
                          const QObject *caller,
                          std::function<void(const QJsonObject &)> successCallback)
{
    util::NetworkRequest req(url);
    req.setCaller(caller);
    req.setRawHeader("Client-ID", clientID.toUtf8());
    req.setRawHeader("Authorization", "OAuth " + oauthToken.toUtf8());
    req.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");

    req.getJSON([=](const QJsonObject &node) {
        successCallback(node);  //
    });
}

static void getUserID(QString username, const QObject *caller,
                      std::function<void(QString)> successCallback)
{
    get("https://api.twitch.tv/kraken/users?login=" + username, caller,
        [=](const QJsonObject &root) {
            if (!root.value("users").isArray()) {
                debug::Log("API Error while getting user id, users is not an array");
                return;
            }

            auto users = root.value("users").toArray();
            if (users.size() != 1) {
                debug::Log("API Error while getting user id, users array size is not 1");
                return;
            }
            if (!users[0].isObject()) {
                debug::Log("API Error while getting user id, first user is not an object");
                return;
            }
            auto firstUser = users[0].toObject();
            auto id = firstUser.value("_id");
            if (!id.isString()) {
                debug::Log("API Error: while getting user id, first user object `_id` key is not a "
                           "string");
                return;
            }
            successCallback(id.toString());
        });
}
static void put(QUrl url, std::function<void(QJsonObject)> successCallback)
{
    QNetworkRequest request(url);

    auto currentTwitchUser = getApp()->accounts->twitch.getCurrent();
    QByteArray oauthToken;
    if (currentTwitchUser) {
        oauthToken = currentTwitchUser->getOAuthToken().toUtf8();
    } else {
        // XXX(pajlada): Bail out?
    }

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Authorization", "OAuth " + oauthToken);

    NetworkManager::urlPut(std::move(request), [=](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            if (!jsonDoc.isNull()) {
                QJsonObject rootNode = jsonDoc.object();

                successCallback(rootNode);
            }
        }
        reply->deleteLater();
    });
}

static void sendDelete(QUrl url, std::function<void()> successCallback)
{
    QNetworkRequest request(url);

    auto currentTwitchUser = getApp()->accounts->twitch.getCurrent();
    QByteArray oauthToken;
    if (currentTwitchUser) {
        oauthToken = currentTwitchUser->getOAuthToken().toUtf8();
    } else {
        // XXX(pajlada): Bail out?
    }

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Authorization", "OAuth " + oauthToken);

    NetworkManager::urlDelete(std::move(request), [=](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            int code =
                reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();

            if (code >= 200 && code <= 299) {
                successCallback();
            }
        }
        reply->deleteLater();
    });
}

}  // namespace twitch
}  // namespace util
}  // namespace chatterino
