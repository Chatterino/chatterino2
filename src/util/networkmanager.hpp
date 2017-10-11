#pragma once

#include <QNetworkAccessManager>
#include <QThread>

namespace chatterino {
namespace messages {

class LazyLoadedImage;

class NetworkWorker : public QObject
{
    Q_OBJECT

public slots:
    void handleRequest(chatterino::messages::LazyLoadedImage *lli, QNetworkAccessManager *nam);
    void handleLoad(LazyLoadedImage *lli, QNetworkReply *reply);

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

    QThread workerThread;
    QNetworkAccessManager NaM;

public:
    NetworkManager();
    ~NetworkManager();

    void queue(chatterino::messages::LazyLoadedImage *lli);
};

}  // namespace messages
}  // namespace chatterino
