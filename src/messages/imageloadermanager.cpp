#include "messages/imageloadermanager.hpp"
#include "emotemanager.hpp"
#include "messages/lazyloadedimage.hpp"
#include "windowmanager.hpp"

#include <QApplication>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace chatterino {
namespace messages {

ImageLoaderManager::ImageLoaderManager()
{
    this->NaM.moveToThread(&this->workerThread);
    this->workerThread.start();
}

ImageLoaderManager::~ImageLoaderManager()
{
    this->workerThread.quit();
    this->workerThread.wait();
}

void ImageLoaderWorker::handleRequest(LazyLoadedImage *lli, QNetworkAccessManager *nam)
{
    QNetworkRequest request(QUrl(lli->getUrl()));
    QNetworkReply *reply = nam->get(request);

    QObject::connect(reply, &QNetworkReply::finished,
                     [lli, reply, this]() { this->handleLoad(lli, reply); });
}

void ImageLoaderManager::queue(chatterino::messages::LazyLoadedImage *lli)
{
    ImageLoaderRequester requester;
    ImageLoaderWorker *worker = new ImageLoaderWorker;

    worker->moveToThread(&this->workerThread);

    QObject::connect(&requester, &ImageLoaderRequester::request, worker,
                     &ImageLoaderWorker::handleRequest);
    QObject::connect(worker, &ImageLoaderWorker::done, lli,
                     [lli]() { lli->windowManager.layoutVisibleChatWidgets(); });

    emit requester.request(lli, &this->NaM);
}

void ImageLoaderWorker::handleLoad(chatterino::messages::LazyLoadedImage *lli, QNetworkReply *reply)
{
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

    reply->deleteLater();
    emit this->done();
    delete this;
}

}  // namespace messages
}  // namespace chatterino
