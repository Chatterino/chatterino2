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
{
    qDebug() << "imageloaderthread" << QThread::currentThread();
    qDebug() << "workerthread: " << &this->workerThread;
    this->NaM->moveToThread(&this->workerThread);
    this->workerThread.start();
}

ImageLoaderManager::~ImageLoaderManager()
{
    this->workerThread.quit();
    this->workerThread.wait();
}

void ImageLoaderWorker::handleRequest(LazyLoadedImage *lli, QNetworkAccessManager *nam)
{
    QNetworkRequest request;
    request.setUrl(QUrl(lli->getUrl()));
    qDebug() << "handleRequest: " << lli->getUrl();
    QNetworkReply *reply = nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished,
                     [lli, reply, this]() { this->handleLoad(lli, reply); });

}

void ImageLoaderManager::queue(chatterino::messages::LazyLoadedImage *lli)
{
    ImageLoaderRequester requester;
    ImageLoaderWorker *workerer = new ImageLoaderWorker;
    workerer->moveToThread(&this->workerThread);
    QObject::connect(&requester, &ImageLoaderRequester::request, workerer,
                     &ImageLoaderWorker::handleRequest);
    emit requester.request(lli, this->NaM);
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
}

}  // namespace messages
}  // namespace chatterino
