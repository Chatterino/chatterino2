#include "messages/imageloader.hpp"
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

ImageLoader::ImageLoader()
{
    std::thread(&ImageLoader::run, this).detach();
}

ImageLoader::~ImageLoader()
{
    {
        std::lock_guard<std::mutex> lk(this->workerM);
        this->exit = true;
        this->ready = true;
    }
    this->cv.notify_all();
}

void ImageLoader::run()
{
    while (true) {
        std::unique_lock<std::mutex> lk(this->workerM);
        if (!this->ready) {
            this->cv.wait(lk, [this]() { return this->ready; });
        }
        if (this->exit) {
            return;
        }
        std::vector<LazyLoadedImage *> current(std::move(this->toProcess));
        // (hemirt)
        // after move toProcess is guaranteed to be empty()
        // and we start processing while more items can be added into toProcess
        this->ready = false;
        lk.unlock();
        for (auto &lli : current) {
            QNetworkRequest request;
            request.setUrl(QUrl(lli->url));
            QNetworkAccessManager NaM;
            QNetworkReply *reply = NaM.get(request);
            QEventLoop eventLoop;
            QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            // Wait until response is read.
            eventLoop.exec();
            qDebug() << "Received emote " << lli->url;
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
        }
    }
}

void ImageLoader::push_back(chatterino::messages::LazyLoadedImage *lli)
{
    {
        std::lock_guard<std::mutex> lk(this->workerM);
        this->toProcess.push_back(lli);
        this->ready = true;
    }
    this->cv.notify_all();
}

}  // namespace messages
}  // namespace chatterino
