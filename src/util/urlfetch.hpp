#pragma once

#include "accountmanager.hpp"
#include "credentials.hpp"

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

#include "networkmanager.hpp"

namespace chatterino {
namespace util {

static void urlFetchJSON(QNetworkRequest request, const QObject *caller,
                         std::function<void(QJsonObject &)> successCallback)
{
    NetworkManager::urlFetch(std::move(request), caller, [=](QNetworkReply *reply) {
        if (reply->error() != QNetworkReply::NetworkError::NoError) {
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject rootNode = jsonDoc.object();

        successCallback(rootNode);
        reply->deleteLater();
    });
}

static void urlFetchJSON(const QString &url, const QObject *caller,
                         std::function<void(QJsonObject &)> successCallback)
{
    urlFetchJSON(QNetworkRequest(QUrl(url)), caller, successCallback);
}

static void urlFetchTimeout(const QString &url, const QObject *caller,
                            std::function<void(QNetworkReply &)> successCallback, int timeoutMs)
{
    QTimer *timer = new QTimer;
    timer->setSingleShot(true);

    QEventLoop *loop = new QEventLoop;

    NetworkManager::urlFetch(QUrl(url), loop,
                             [=](QNetworkReply *reply) {
                                 if (reply->error() == QNetworkReply::NetworkError::NoError) {
                                     // qDebug() << "successCallback";
                                     successCallback(*reply);
                                 }

                                 reply->deleteLater();
                                 loop->quit();
                             },
                             [loop, timer](QNetworkReply *reply) {
                                 qDebug() << "connect fun";
                                 QObject::connect(timer, &QTimer::timeout, loop, [=]() {
                                     qDebug() << "TIMED OUT";
                                     QObject::disconnect(reply, &QNetworkReply::finished, loop,
                                                         &QEventLoop::quit);
                                     reply->abort();
                                     reply->deleteLater();
                                 });
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
                    [=](QNetworkReply &reply) {
                        QByteArray data = reply.readAll();
                        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

                        if (jsonDoc.isNull()) {
                            return;
                        }

                        QJsonObject rootNode = jsonDoc.object();
                        // qDebug() << "rootnode\n" << rootNode;
                        successCallback(rootNode);
                    },
                    timeoutMs);
}

namespace twitch {

template <typename Callback>
static void get(QString url, std::function<void(QJsonObject &)> successCallback,
                const QObject *caller, Callback callback)
{
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    urlFetchJSON(std::move(request), caller, successCallback);
}

static void get(QString url, const QObject *caller,
                std::function<void(QJsonObject &)> successCallback)
{
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    urlFetchJSON(std::move(request), caller, successCallback);
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
