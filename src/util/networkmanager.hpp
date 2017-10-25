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

public slots:
    void handleRequest(chatterino::messages::LazyLoadedImage *lli);
    void handleLoad(chatterino::messages::LazyLoadedImage *lli, QNetworkReply *reply);

signals:
    void done();
    void doneUrl(QNetworkReply *);
};

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void request(chatterino::messages::LazyLoadedImage *lli);
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
                                              [fun, reply]() { fun(reply); });
                         });

        emit requester.requestUrl();
    }

    template <typename Fun>
    static void urlFetch(const QUrl &url, Fun fun)
    {
        urlFetch(QNetworkRequest(url), std::move(fun));
    }

    // (hemirt) experimental, no tests done
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

        QObject::connect(worker, &NetworkWorker::doneUrl, caller,
                         [=](QNetworkReply *reply) { callback(reply); });
        emit requester.requestUrl();
    }

    template <typename Callback, typename Connectoid = void (*)(QNetworkReply *)>
    static void urlFetch(const QUrl &url, const QObject *caller, Callback callback,
                         Connectoid connectFun = [](QNetworkReply *) { return; })
    {
        urlFetch(QNetworkRequest(url), caller, callback, connectFun);
    }
};

}  // namespace util
}  // namespace chatterino
