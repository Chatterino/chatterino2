#include "messages/lazyloadedimage.h"

#include "asyncexec.h"
#include "emotes.h"
#include "ircmanager.h"
#include "windows.h"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : currentPixmap(NULL)
    , allFrames()
    , currentFrame(0)
    , url(url)
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(QPixmap *image, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : currentPixmap(image)
    , allFrames()
    , currentFrame(0)
    , url()
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(true)
{
}

void
LazyLoadedImage::loadImage()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url(this->url);
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
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
                    this->currentPixmap = pixmap;
                }

                FrameData data;
                data.duration = std::max(20, reader.nextImageDelay());
                data.image = pixmap;

                allFrames.push_back(data);
            }
        }

        if (allFrames.size() > 1) {
            QObject::connect(&Emotes::getGifUpdateTimer(), &QTimer::timeout,
                             [this] { gifUpdateTimout(); });
        }

        Emotes::incGeneration();
        Windows::layoutVisibleChatWidgets();

        reply->deleteLater();
        manager->deleteLater();
    });
}

void
LazyLoadedImage::gifUpdateTimout()
{
    if (this->currentFrame >= this->allFrames.size() - 1) {
        this->currentFrame = 0;
        this->currentPixmap = this->allFrames.at(0).image;
    } else {
        this->currentPixmap = this->allFrames.at(++this->currentFrame).image;
    }
}
}
}
