#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QThread>
#include <QTimerEvent>

namespace chatterino {
namespace messages {

class LazyLoadedImage;

class ImageLoaderWorker : public QObject
{
    Q_OBJECT
public:
public slots:
    void handleRequest(chatterino::messages::LazyLoadedImage *lli, QNetworkAccessManager *nam);
    void handleLoad(LazyLoadedImage *lli, QNetworkReply *reply);
};

class ImageLoaderRequester : public QObject
{
    Q_OBJECT
signals:
    void request(chatterino::messages::LazyLoadedImage *lli, QNetworkAccessManager *nam);
};

class ImageLoaderManager : public QObject
{
    Q_OBJECT
    QThread workerThread;
    QNetworkAccessManager *NaM;

public:
    ImageLoaderManager();
    ~ImageLoaderManager();
    void queue(chatterino::messages::LazyLoadedImage *lli);
};

}  // namespace messages
}  // namespace chatterino
