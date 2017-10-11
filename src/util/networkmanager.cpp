#include "util/networkmanager.hpp"
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

NetworkManager::NetworkManager()
{
    this->NaM.moveToThread(&this->workerThread);
    this->workerThread.start();
}

NetworkManager::~NetworkManager()
{
    this->workerThread.quit();
    this->workerThread.wait();
}

void NetworkWorker::handleRequest(LazyLoadedImage *lli, QNetworkAccessManager *nam)
{
    QNetworkRequest request(QUrl(lli->getUrl()));
    QNetworkReply *reply = nam->get(request);

    QObject::connect(reply, &QNetworkReply::finished,
                     [lli, reply, this]() { this->handleLoad(lli, reply); });
}

void NetworkManager::queue(chatterino::messages::LazyLoadedImage *lli)
{
    NetworkRequester requester;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(&this->workerThread);

    QObject::connect(&requester, &NetworkRequester::request, worker,
                     &NetworkWorker::handleRequest);
    QObject::connect(worker, &NetworkWorker::done, lli,
                     [lli]() { lli->windowManager.layoutVisibleChatWidgets(); });

    emit requester.request(lli, &this->NaM);
}

void NetworkWorker::handleLoad(chatterino::messages::LazyLoadedImage *lli, QNetworkReply *reply)
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
