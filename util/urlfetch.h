#ifndef URLFETCH_H
#define URLFETCH_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include <functional>

namespace chatterino {
namespace util {

static void urlFetch(const QString &url, std::function<void(QNetworkReply &)> successCallback)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            successCallback(*reply);
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

static void urlJsonFetch(const QString &url, std::function<void(QJsonObject &)> successCallback)
{
    urlFetch(url, [=](QNetworkReply &reply) {
        QByteArray data = reply.readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

        if (jsonDoc.isNull()) {
            return;
        }

        QJsonObject rootNode = jsonDoc.object();

        successCallback(rootNode);
    });
}
}
}

#endif  // URLFETCH_H
