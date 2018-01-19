#pragma once

#include "debug/log.hpp"
#include "util/networkrequester.hpp"
#include "util/networkworker.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace chatterino {
namespace util {

static QJsonObject parseJSONFromReplyxD(QNetworkReply *reply)
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

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static QThread workerThread;
    static QNetworkAccessManager NaM;

    static void init();
    static void deinit();

    template <typename FinishedCallback>
    static void urlFetch(QNetworkRequest request, FinishedCallback onFinished)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [ worker, onFinished = std::move(onFinished), request = std::move(request) ]() {
                QNetworkReply *reply = NetworkManager::NaM.get(request);

                QObject::connect(reply, &QNetworkReply::finished,
                                 [ worker, reply, onFinished = std::move(onFinished) ]() {
                                     onFinished(reply);
                                     delete worker;
                                 });
            });

        emit requester.requestUrl();
    }

    template <typename FinishedCallback>
    static void urlFetch(const QUrl &url, FinishedCallback onFinished)
    {
        urlFetch(QNetworkRequest(url), std::move(onFinished));
    }

    template <typename Callback, typename ReplyCreatedCallback = void (*)(QNetworkReply *)>
    static void urlFetch(QNetworkRequest request, const QObject *caller, Callback callback,
                         ReplyCreatedCallback onReplyCreated = [](QNetworkReply *) { return; })
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        QObject::connect(&requester, &NetworkRequester::requestUrl, worker, [=]() {
            QNetworkReply *reply = NetworkManager::NaM.get(request);

            onReplyCreated(reply);

            QObject::connect(reply, &QNetworkReply::finished, worker,
                             [=]() { emit worker->doneUrl(reply); });
        });

        QObject::connect(worker, &NetworkWorker::doneUrl, caller, [=](QNetworkReply *reply) {
            callback(reply);
            delete worker;
        });
        emit requester.requestUrl();
    }

    template <typename Callback, typename ReplyCreatedCallback = void (*)(QNetworkReply *)>
    static void urlFetch(const QUrl &url, const QObject *caller, Callback callback,
                         ReplyCreatedCallback onReplyCreated = [](QNetworkReply *) { return; })
    {
        urlFetch(QNetworkRequest(url), caller, callback, onReplyCreated);
    }

    template <typename FinishedCallback>
    static void urlPut(QNetworkRequest request, FinishedCallback onFinished, QByteArray *data)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [ worker, data, onFinished = std::move(onFinished), request = std::move(request) ]() {
                QNetworkReply *reply = NetworkManager::NaM.put(request, *data);

                QObject::connect(reply, &QNetworkReply::finished,
                                 [ worker, reply, onFinished = std::move(onFinished) ]() {
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
            [ onFinished = std::move(onFinished), request = std::move(request), worker ]() {
                QNetworkReply *reply = NetworkManager::NaM.put(request, "");

                QObject::connect(reply, &QNetworkReply::finished,
                                 [ onFinished = std::move(onFinished), reply, worker ]() {
                                     onFinished(reply);
                                     delete worker;
                                 });
            });

        emit requester.requestUrl();
    }

    template <typename FinishedCallback>
    static void urlDelete(QNetworkRequest request, FinishedCallback onFinished)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [ onFinished = std::move(onFinished), request = std::move(request), worker ]() {
                QNetworkReply *reply = NetworkManager::NaM.deleteResource(request);

                QObject::connect(reply, &QNetworkReply::finished,
                                 [ onFinished = std::move(onFinished), reply, worker ]() {
                                     onFinished(reply);
                                     delete worker;
                                 });
            });

        emit requester.requestUrl();
    }
};

}  // namespace util
}  // namespace chatterino
