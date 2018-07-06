#pragma once

#include "common/NetworkManager.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchCommon.hpp"

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

static void twitchApiGet(QString url, const QObject *caller,
                         std::function<void(const QJsonObject &)> successCallback)
{
    NetworkRequest req(url);
    req.setCaller(caller);
    req.setRawHeader("Client-ID", getDefaultClientID());
    req.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");

    req.getJSON([=](const QJsonObject &node) {
        successCallback(node);  //
    });
}

static void twitchApiGet2(QString url, const QObject *caller, bool useQuickLoadCache,
                          std::function<void(const rapidjson::Document &)> successCallback)
{
    NetworkRequest request(url);
    request.setRequestType(NetworkRequest::GetRequest);
    request.setCaller(caller);
    request.makeAuthorizedV5(getDefaultClientID());
    request.setUseQuickLoadCache(useQuickLoadCache);

    request.onSuccess([successCallback](const rapidjson::Document &document) {
        successCallback(document);  //

        return true;
    });

    request.execute();
}

static void twitchApiGetUserID(QString username, const QObject *caller,
                               std::function<void(QString)> successCallback)
{
    twitchApiGet(
        "https://api.twitch.tv/kraken/users?login=" + username, caller,
        [=](const QJsonObject &root) {
            if (!root.value("users").isArray()) {
                Log("API Error while getting user id, users is not an array");
                return;
            }

            auto users = root.value("users").toArray();
            if (users.size() != 1) {
                Log("API Error while getting user id, users array size is not 1");
                return;
            }
            if (!users[0].isObject()) {
                Log("API Error while getting user id, first user is not an object");
                return;
            }
            auto firstUser = users[0].toObject();
            auto id = firstUser.value("_id");
            if (!id.isString()) {
                Log("API Error: while getting user id, first user object `_id` key is not a "
                    "string");
                return;
            }
            successCallback(id.toString());
        });
}
static void twitchApiPut(QUrl url, std::function<void(const rapidjson::Document &)> successCallback)
{
    NetworkRequest request(url);
    request.setRequestType(NetworkRequest::PutRequest);
    request.setCaller(QThread::currentThread());

    auto currentTwitchUser = getApp()->accounts->twitch.getCurrent();
    QByteArray oauthToken;
    if (currentTwitchUser) {
        oauthToken = currentTwitchUser->getOAuthToken().toUtf8();
    } else {
        // XXX(pajlada): Bail out?
    }

    request.makeAuthorizedV5(getDefaultClientID(), currentTwitchUser->getOAuthToken());

    request.onSuccess([successCallback](const auto &document) {
        if (!document.IsNull()) {
            successCallback(document);
        }

        return true;
    });

    request.execute();
}

static void twitchApiDelete(QUrl url, std::function<void()> successCallback)
{
    NetworkRequest request(url);
    request.setRequestType(NetworkRequest::DeleteRequest);
    request.setCaller(QThread::currentThread());

    auto currentTwitchUser = getApp()->accounts->twitch.getCurrent();
    QByteArray oauthToken;
    if (currentTwitchUser) {
        oauthToken = currentTwitchUser->getOAuthToken().toUtf8();
    } else {
        // XXX(pajlada): Bail out?
    }

    request.makeAuthorizedV5(getDefaultClientID(), currentTwitchUser->getOAuthToken());

    request.onError([successCallback](int code) {
        if (code >= 200 && code <= 299) {
            successCallback();
        }

        return true;
    });

    request.onSuccess([successCallback](const auto &document) {
        successCallback();

        return true;
    });

    request.execute();
}

}  // namespace chatterino
