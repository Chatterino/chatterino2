#include "messages/lazyloadedimage.hpp"
#include "asyncexec.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <thread>

#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                                 const QString &url, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : emoteManager(_emoteManager)
    , currentPixmap(nullptr)
    , url(url)
    , name(name)
    , tooltip(tooltip)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                                 QPixmap *image, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : emoteManager(_emoteManager)
    , currentPixmap(image)
    , name(name)
    , tooltip(tooltip)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(true)
{
}

void LazyLoadedImage::loadImage()
{
    std::thread([=]() {
        QNetworkRequest request;
        request.setUrl(QUrl(this->url));
        QNetworkAccessManager NaM;
        QEventLoop eventLoop;
        QNetworkReply *reply = NaM.get(request);
        QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();  // Wait until response is read.

        qDebug() << "Received emote " << this->url;
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

                this->allFrames.push_back(data);
            }
        }

        if (this->allFrames.size() > 1) {
            this->animated = true;
        }

        this->emoteManager.incGeneration();

        delete reply;
    })
        .detach();
}

const QPixmap *LazyLoadedImage::getPixmap()
{
    if (!this->isLoading) {
        this->isLoading = true;

        loadImage();
    }
    return this->currentPixmap;
}

qreal LazyLoadedImage::getScale() const
{
    return this->scale;
}

const QString &LazyLoadedImage::getUrl() const
{
    return this->url;
}

const QString &LazyLoadedImage::getName() const
{
    return this->name;
}

const QString &LazyLoadedImage::getTooltip() const
{
    return this->tooltip;
}

const QMargins &LazyLoadedImage::getMargin() const
{
    return this->margin;
}

bool LazyLoadedImage::getAnimated() const
{
    return this->animated;
}

bool LazyLoadedImage::isHat() const
{
    return this->ishat;
}

int LazyLoadedImage::getWidth() const
{
    if (this->currentPixmap == nullptr) {
        return 16;
    }
    return this->currentPixmap->width();
}

int LazyLoadedImage::getScaledWidth() const
{
    return static_cast<int>(getWidth() * this->scale);
}

int LazyLoadedImage::getHeight() const
{
    if (this->currentPixmap == nullptr) {
        return 16;
    }
    return this->currentPixmap->height();
}

int LazyLoadedImage::getScaledHeight() const
{
    return static_cast<int>(getHeight() * this->scale);
}

}  // namespace messages
}  // namespace chatterino
