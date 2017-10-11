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
namespace util {

QThread NetworkManager::workerThread;
QNetworkAccessManager NetworkManager::NaM;

void NetworkManager::init()
{
    NetworkManager::NaM.moveToThread(&NetworkManager::workerThread);
    NetworkManager::workerThread.start();
}

void NetworkManager::deinit()
{
    NetworkManager::workerThread.quit();
    NetworkManager::workerThread.wait();
}

void NetworkWorker::handleRequest(chatterino::messages::LazyLoadedImage *lli,
                                  QNetworkAccessManager *nam)
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

    worker->moveToThread(&NetworkManager::workerThread);

    QObject::connect(&requester, &NetworkRequester::request, worker, &NetworkWorker::handleRequest);
    QObject::connect(worker, &NetworkWorker::done, lli,
                     [lli]() { lli->windowManager.layoutVisibleChatWidgets(); });

    emit requester.request(lli, &NetworkManager::NaM);
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

            chatterino::messages::LazyLoadedImage::FrameData data;
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

}  // namespace util
}  // namespace chatterino
