#pragma once

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

namespace chatterino {
namespace util {

static void urlFetch(const QString &url, std::function<void(QNetworkReply &)> successCallback,
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
}

static void urlFetchJSON(const QString &url, std::function<void(QJsonObject &)> successCallback,
                         QNetworkAccessManager *manager = nullptr)
{
    urlFetch(url,
             [=](QNetworkReply &reply) {
                 QByteArray data = reply.readAll();
                 QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

                 if (jsonDoc.isNull()) {
                     return;
                 }

                 QJsonObject rootNode = jsonDoc.object();

                 successCallback(rootNode);
             },
             manager);
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
