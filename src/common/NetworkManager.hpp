#pragma once

#include "common/NetworkRequester.hpp"
#include "common/NetworkWorker.hpp"
#include "debug/Log.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace chatterino {

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static QThread workerThread;
    static QNetworkAccessManager NaM;

    static void init();
    static void deinit();

    template <typename FinishedCallback>
    static void urlPut(QNetworkRequest request, FinishedCallback onFinished, QByteArray *data)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [worker, data, onFinished = std::move(onFinished), request = std::move(request)]() {
                QNetworkReply *reply = NetworkManager::NaM.put(request, *data);

                reply->connect(reply, &QNetworkReply::finished,
                               [worker, reply, onFinished = std::move(onFinished)]() {
                                   onFinished(reply);
                                   delete worker;
                               });
            });

        emit requester.requestUrl();
    }

    template <typename FinishedCallback>
    static void urlPut(QNetworkRequest request, FinishedCallback onFinished)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [onFinished = std::move(onFinished), request = std::move(request), worker]() {
                QNetworkReply *reply = NetworkManager::NaM.put(request, "");

                reply->connect(reply, &QNetworkReply::finished,
                               [onFinished = std::move(onFinished), reply, worker]() {
                                   onFinished(reply);
                                   delete worker;
                               });
            });

        emit requester.requestUrl();
    }
};

}  // namespace chatterino
