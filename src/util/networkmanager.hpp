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
    static void queue(const QUrl &url, Fun fun)
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

    template <typename Fun, typename Callback>
    static void queue(const QUrl &url, Fun fun, const QObject *caller, Callback &&callback)
    {
        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        QObject::connect(&requester, &NetworkRequester::requestUrl, worker, [fun, url, caller]() {
            QNetworkRequest request(url);
            QNetworkReply *reply = NetworkManager::NaM.get(request);

            QObject::connect(reply, &QNetworkReply::finished, caller,
                             [fun, reply]() { fun(reply); });
        });

        QObject::connect(worker, &NetworkWorker::done, caller, std::forward<Callback>(callback));

        emit requester.requestUrl();
    }
};

}  // namespace util
}  // namespace chatterino
