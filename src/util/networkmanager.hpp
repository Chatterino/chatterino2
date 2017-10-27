#pragma once

#include "debug/log.hpp"

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

class NetworkRequest
{
    struct Data {
        QNetworkRequest request;
        const QObject *caller = nullptr;
        std::function<void(QNetworkReply *)> onReplyCreated;
        int timeoutMS = -1;
    } data;

public:
    NetworkRequest() = delete;

    explicit NetworkRequest(const char *url)
    {
        this->data.request.setUrl(QUrl(url));
    }

    explicit NetworkRequest(const std::string &url)
    {
        this->data.request.setUrl(QUrl(QString::fromStdString(url)));
    }

    explicit NetworkRequest(const QString &url)
    {
        this->data.request.setUrl(QUrl(url));
    }

    void setCaller(const QObject *_caller)
    {
        this->data.caller = _caller;
    }

    void setOnReplyCreated(std::function<void(QNetworkReply *)> f)
    {
        this->data.onReplyCreated = f;
    }

    void setRawHeader(const QByteArray &headerName, const QByteArray &value)
    {
        this->data.request.setRawHeader(headerName, value);
    }

    void setTimeout(int ms)
    {
        this->data.timeoutMS = ms;
    }

    template <typename FinishedCallback>
    void get(FinishedCallback onFinished)
    {
        QTimer *timer = nullptr;
        if (this->data.timeoutMS > 0) {
            timer = new QTimer;
        }

        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        if (this->data.caller != nullptr) {
            QObject::connect(worker, &NetworkWorker::doneUrl, this->data.caller,
                             [onFinished](auto reply) {
                                 onFinished(reply);
                                 reply->deleteLater();
                             });
        }

        if (timer != nullptr) {
            timer->start(this->data.timeoutMS);
        }

        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [ timer, data = std::move(this->data), worker, onFinished{std::move(onFinished)} ]() {
                QNetworkReply *reply = NetworkManager::NaM.get(data.request);

                if (timer != nullptr) {
                    QObject::connect(timer, &QTimer::timeout, worker, [reply, timer]() {
                        debug::Log("Aborted!");
                        reply->abort();
                        timer->deleteLater();
                    });
                }

                if (data.onReplyCreated) {
                    data.onReplyCreated(reply);
                }

                QObject::connect(reply, &QNetworkReply::finished, worker,
                                 [ data = std::move(this->data), worker, reply, onFinished ]() {
                                     if (data.caller == nullptr) {
                                         onFinished(reply);

                                         reply->deleteLater();
                                     } else {
                                         emit worker->doneUrl(reply);
                                     }

                                     delete worker;
                                 });
            });

        emit requester.requestUrl();
    }

    template <typename FinishedCallback>
    void getJSON(FinishedCallback onFinished)
    {
        this->get([onFinished{std::move(onFinished)}](auto reply) {
            auto object = parseJSONFromReplyxD(reply);
            onFinished(object);
        });
    }
};

class NetworkWorker : public QObject
{
    Q_OBJECT

signals:
    void doneUrl(QNetworkReply *);
};

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void requestUrl();
};

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
        QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                         [ onFinished = std::move(onFinished), request = std::move(request) ]() {
                             QNetworkReply *reply = NetworkManager::NaM.get(request);

                             QObject::connect(reply, &QNetworkReply::finished,
                                              [onFinished, reply, worker]() {
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
            [ onFinished = std::move(onFinished), request = std::move(request), data ]() {
                QNetworkReply *reply = NetworkManager::NaM.put(request, *data);

                QObject::connect(reply, &QNetworkReply::finished,
                                 [ onFinished = std::move(onFinished), reply ]() {
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
};

}  // namespace util
}  // namespace chatterino
