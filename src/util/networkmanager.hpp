#pragma once

#include <QNetworkAccessManager>
#include <QThread>

namespace chatterino {

namespace messages {

class LazyLoadedImage;

}  // namespace messages

namespace util {

class NetworkWorker : public QObject
{
    Q_OBJECT

public slots:
    void handleRequest(chatterino::messages::LazyLoadedImage *lli, QNetworkAccessManager *nam);
    void handleLoad(chatterino::messages::LazyLoadedImage *lli, QNetworkReply *reply);

signals:
    void done();
};

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void request(chatterino::messages::LazyLoadedImage *lli, QNetworkAccessManager *nam);
};

class NetworkManager : public QObject
{
    Q_OBJECT

    static QThread workerThread;
    static QNetworkAccessManager NaM;

public:
    static void init();
    static void deinit();

    static void queue(chatterino::messages::LazyLoadedImage *lli);
};

}  // namespace util
}  // namespace chatterino
