#pragma once

#include <QNetworkAccessManager>
#include <QThread>
#include <QUrl>

namespace chatterino {

namespace messages {

class LazyLoadedImage;

}  // namespace messages

namespace util {

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

    static void queue(chatterino::messages::LazyLoadedImage *lli);

    template <typename Fun>
    static void urlFetch(QNetworkRequest request, Fun fun)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                         [ fun = std::move(fun), request = std::move(request) ]() {
                             QNetworkReply *reply = NetworkManager::NaM.get(request);

                             QObject::connect(reply, &QNetworkReply::finished,
                                              [fun, reply, worker]() {
                                                  fun(reply);
                                                  delete worker;
                                              });
                         });

        emit requester.requestUrl();
    }

    template <typename Fun>
    static void urlFetch(const QUrl &url, Fun fun)
    {
        urlFetch(QNetworkRequest(url), std::move(fun));
    }

    template <typename Callback, typename Connectoid = void (*)(QNetworkReply *)>
    static void urlFetch(QNetworkRequest request, const QObject *caller, Callback callback,
                         Connectoid connectFun = [](QNetworkReply *) { return; })
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        QObject::connect(&requester, &NetworkRequester::requestUrl, worker, [=]() {
            QNetworkReply *reply = NetworkManager::NaM.get(request);

            connectFun(reply);

            QObject::connect(reply, &QNetworkReply::finished, worker,
                             [=]() { emit worker->doneUrl(reply); });
        });

        QObject::connect(worker, &NetworkWorker::doneUrl, caller, [=](QNetworkReply *reply) {
            callback(reply);
            delete worker;
        });
        emit requester.requestUrl();
    }

    template <typename Callback, typename Connectoid = void (*)(QNetworkReply *)>
    static void urlFetch(const QUrl &url, const QObject *caller, Callback callback,
                         Connectoid connectFun = [](QNetworkReply *) { return; })
    {
        urlFetch(QNetworkRequest(url), caller, callback, connectFun);
    }

    template <typename Fun>
    static void urlPut(QNetworkRequest request, Fun fun, QByteArray *data)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                         [ fun = std::move(fun), request = std::move(request), data ]() {
                             QNetworkReply *reply = NetworkManager::NaM.put(request, *data);

                             QObject::connect(reply, &QNetworkReply::finished,
                                              [ fun = std::move(fun), reply ]() {
                                                  fun(reply);
                                                  delete worker;
                                              });
                         });

        emit requester.requestUrl();
    }

    template <typename Fun>
    static void urlPut(QNetworkRequest request, Fun fun)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                         [ fun = std::move(fun), request = std::move(request), worker ]() {
                             QNetworkReply *reply = NetworkManager::NaM.put(request, "");

                             QObject::connect(reply, &QNetworkReply::finished,
                                              [ fun = std::move(fun), reply, worker ]() {
                                                  fun(reply);
                                                  delete worker;
                                              });
                         });

        emit requester.requestUrl();
    }
};

}  // namespace util
}  // namespace chatterino
