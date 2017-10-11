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

#include <functional>

#include "networkmanager.hpp"

namespace chatterino {
namespace util {

namespace twitch {

static void get(QString url, std::function<void(QJsonObject &)> successCallback)
{
    auto manager = new QNetworkAccessManager();

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);

    request.setRawHeader("Client-ID", getDefaultClientID());
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

            if (!jsonDoc.isNull()) {
                QJsonObject rootNode = jsonDoc.object();

                successCallback(rootNode);
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

static void getUserID(QString username, std::function<void(QString)> successCallback)
{
    get("https://api.twitch.tv/kraken/users?login=" + username, [=](const QJsonObject &root) {
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
            qDebug()
                << "API Error: while getting user id, first user object `_id` key is not a string";
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
    QNetworkReply *reply = manager->put(request, "");
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            if (!jsonDoc.isNull()) {
                QJsonObject rootNode = jsonDoc.object();

                successCallback(rootNode);
            }
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

}  // namespace twitch

static void urlFetchJSON(const QString &url, std::function<void(QJsonObject &)> successCallback,
                         QNetworkAccessManager *manager = nullptr)
{
    chatterino::util::NetworkManager::queue(QUrl(url), [=](QNetworkReply *reply) {
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

static void urlFetchTimeout(const QString &url,
                            std::function<void(QNetworkReply &)> successCallback, int timeoutMs,
                            QNetworkAccessManager *manager = nullptr)
{
    bool customManager = true;

    if (manager == nullptr) {
        manager = new QNetworkAccessManager();
        customManager = false;
    }

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);

    QNetworkReply *reply = manager->get(request);

    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(reply, &QNetworkReply::finished, [=] {
        /* uncomment to follow redirects
        QVariant replyStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if (replyStatus >= 300 && replyStatus <= 304) {
            QString newUrl =
                reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
            urlFetch(newUrl, successCallback);
            return;
        }
        */

        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            successCallback(*reply);
        }

        reply->deleteLater();
        if (!customManager) {
            manager->deleteLater();
        }
    });
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        qDebug() << "TIMED OUT";
        QObject::disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        reply->abort();
    } else {
        // qDebug() << "XDDD HEHEHE";
    }
}

static void urlFetchJSONTimeout(const QString &url,
                                std::function<void(QJsonObject &)> successCallback, int timeoutMs,
                                QNetworkAccessManager *manager = nullptr)
{
    urlFetchTimeout(url,
                    [=](QNetworkReply &reply) {
                        QByteArray data = reply.readAll();
                        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

                        if (jsonDoc.isNull()) {
                            return;
                        }

                        QJsonObject rootNode = jsonDoc.object();

                        successCallback(rootNode);
                    },
                    timeoutMs, manager);
}

}  // namespace util
}  // namespace chatterino
