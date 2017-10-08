#include "messages/imageloadermanager.hpp"
#include "emotemanager.hpp"
#include "messages/lazyloadedimage.hpp"
#include "windowmanager.hpp"

#include <QBuffer>
#include <QDebug>
#include <QEventLoop>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <sstream>

namespace chatterino {
namespace messages {

ImageLoaderManager::ImageLoaderManager()
    : NaM(new QNetworkAccessManager)
    , worker(new ImageLoaderWorker)
{
    qDebug() << "imagloead" << QThread::currentThread();
    qDebug() << "worker: " << &this->workerThread;
    this->NaM->moveToThread(&this->workerThread);
    this->worker->moveToThread(&this->workerThread);
    qDebug() << "555;";
    QObject::connect(this, &ImageLoaderManager::request, this->worker,
                     &ImageLoaderWorker::handleRequest);
    qDebug() << "222";
    this->workerThread.start();
}

ImageLoaderManager::~ImageLoaderManager()
{
    this->workerThread.quit();
}

void ImageLoaderWorker::handleRequest(LazyLoadedImage *lli, QNetworkAccessManager *nam)
{
    QNetworkRequest request;
    request.setUrl(QUrl(lli->getUrl()));
    qDebug() << "keepo";
    QNetworkReply *reply = nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished,
                     [lli, reply, this]() { this->handleLoad(lli, reply); });

}

void ImageLoaderManager::queue(chatterino::messages::LazyLoadedImage *lli)
{
    /*
    ImageLoaderWorker *worker = new ImageLoaderWorker(lli);
    qDebug() << "queue: " << QThread::currentThread();
    QNetworkRequest request;
    request.setUrl(QUrl(lli->getUrl()));
    qDebug() << "keepo";
    worker->reply = this->NaM.get(request);
    qDebug() << "kappa";
    worker->moveToThread(&this->workerThread);
    /*QObject::connect(worker->reply, &QNetworkReply::finished, worker,
                     &ImageLoaderWorker::handleLoad, Qt::ConnectionType::QueuedConnection);
    */
    /*QObject::connect(worker->reply, &QNetworkReply::finished, worker,
                     [worker](){worker->handleLoad();});
    */
    emit request(lli, this->NaM);
    qDebug() << lli->getUrl();
}

void ImageLoaderWorker::handleLoad(chatterino::messages::LazyLoadedImage *lli, QNetworkReply *reply)
{
    qDebug() << "Received emote " << lli->url;
    qDebug() << QThread::currentThread();
    QByteArray array = reply->readAll();
    QBuffer buffer(&array);
    buffer.open(QIODevice::ReadOnly);

    QImage image;
    QImageReader reader(&buffer);

    bool first = true;

    for (int index = 0; index < reader.imageCount(); ++index) {
        if (reader.read(&image)) {
            auto pixmap = new QPixmap(QPixmap::fromImage(image));

            if (first) {
                first = false;
                lli->currentPixmap = pixmap;
            }

            LazyLoadedImage::FrameData data;
            data.duration = std::max(20, reader.nextImageDelay());
            data.image = pixmap;

            lli->allFrames.push_back(data);
        }
    }

    if (lli->allFrames.size() > 1) {
        lli->animated = true;
    }

    lli->emoteManager.incGeneration();
    lli->windowManager.layoutVisibleChatWidgets();

    delete reply;
    delete this;
    qDebug() << "Keeeeeee";
}

}  // namespace messages
}  // namespace chatterino
