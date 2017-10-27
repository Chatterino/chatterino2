#pragma once

#include "accountmanager.hpp"
#include "credentials.hpp"
#include "util/networkmanager.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QTimer>

#include <QDebug>

#include <functional>

namespace chatterino {
namespace util {

static QJsonObject parseJSONFromReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        return QJsonObject();
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

    if (jsonDoc.isNull()) {
        return QJsonObject();
    }

    return jsonDoc.object();
}

static void urlFetchTimeout(const QString &url, const QObject *caller,
                            std::function<void(QNetworkReply *)> successCallback, int timeoutMs)
{
    QTimer *timer = new QTimer;
    timer->setSingleShot(true);

    QEventLoop *loop = new QEventLoop;

    util::NetworkRequest req(url);
    req.setCaller(loop);
    req.setOnReplyCreated([loop, timer](QNetworkReply *reply) {
        QObject::connect(timer, &QTimer::timeout, loop, [=]() {
            QObject::disconnect(reply, &QNetworkReply::finished, loop, &QEventLoop::quit);
            reply->abort();
            reply->deleteLater();
        });
    });
    req.get([=](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            successCallback(reply);
        }

        reply->deleteLater();
        loop->quit();
    });

    QObject::connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));

    timer->start(timeoutMs);
    loop->exec();
    delete timer;
    delete loop;
}

static void urlFetchJSONTimeout(const QString &url, const QObject *caller,
                                std::function<void(QJsonObject &)> successCallback, int timeoutMs)
{
    urlFetchTimeout(url, caller,
                    [=](QNetworkReply *reply) {
                        auto node = parseJSONFromReply(reply);
                        successCallback(node);
                    },
                    timeoutMs);
}

namespace twitch {

static void get(QString url, const QObject *caller,
                std::function<void(QJsonObject &)> successCallback)
{
    util::NetworkRequest req(url);
    req.setCaller(caller);
    req.setRawHeader("Client-ID", getDefaultClientID());
    req.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    req.get([=](QNetworkReply *reply) {
        auto node = parseJSONFromReply(reply);
        successCallback(node);
    });
}

static void getUserID(QString username, const QObject *caller,
                      std::function<void(QString)> successCallback)
{
    get("https://api.twitch.tv/kraken/users?login=" + username, caller,
        [=](const QJsonObject &root) {
            if (!root.value("users").isArray()) {
                qDebug() << "API Error while getting user id, users is not an array";
                return;
            }

            auto users = root.value("users").toArray();
            if (users.size() != 1) {
                qDebug() << "API Error while getting user id, users array size is not 1";
                return;
            }
            if (!users[0].isObject()) {
                qDebug() << "API Error while getting user id, first user is not an object";
                return;
            }
            auto firstUser = users[0].toObject();
            auto id = firstUser.value("_id");
            if (!id.isString()) {
                qDebug() << "API Error: while getting user id, first user object `_id` key is not "
                            "a string";
                return;
            }
            successCallback(id.toString());
        });
}
static void put(QUrl url, std::function<void(QJsonObject)> successCallback)
{
    auto manager = new QNetworkAccessManager();
    QNetworkRequest request(url);

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader(
        "Authorization",
        "OAuth " + AccountManager::getInstance().getTwitchUser().getOAuthToken().toUtf8());

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

}  // namespace twitch
}  // namespace util
}  // namespace chatterino
