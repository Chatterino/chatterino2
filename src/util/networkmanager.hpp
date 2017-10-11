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
    static void urlFetch(const QUrl &url, Fun fun)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);
        QObject::connect(&requester, &NetworkRequester::requestUrl, worker, [fun, url]() {
            QNetworkRequest request(url);
            QNetworkReply *reply = NetworkManager::NaM.get(request);

            QObject::connect(reply, &QNetworkReply::finished, [fun, reply]() { fun(reply); });
        });

        emit requester.requestUrl();
    }

    // (hemirt) experimental, no tests done
    template <typename Fun, typename Callback, typename Connectoid>
    static void urlFetch(const QUrl &url, Fun fun, const QObject *caller, Callback callback,
                         Connectoid connectFun = [](QNetworkReply*){return;})
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        QObject::connect(&requester, &NetworkRequester::requestUrl, worker, [=]() {
            QNetworkRequest request(url);
            QNetworkReply *reply = NetworkManager::NaM.get(request);

            connectFun(reply);

            QObject::connect(reply, &QNetworkReply::finished, worker, [=]() {
                emit worker->done();
                fun(reply);
            });
        });

        QObject::connect(worker, &NetworkWorker::done, caller, [=]() { callback(); });

        emit requester.requestUrl();
    }
};

}  // namespace util
}  // namespace chatterino
