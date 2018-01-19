#include "messages/image.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/networkmanager.hpp"
#include "util/posttothread.hpp"
#include "util/urlfetch.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <functional>
#include <thread>

namespace chatterino {
namespace messages {

Image::Image(const QString &url, qreal scale, const QString &name, const QString &tooltip,
             const QMargins &margin, bool isHat)
    : currentPixmap(nullptr)
    , url(url)
    , name(name)
    , tooltip(tooltip)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(false)
{
}

Image::Image(QPixmap *image, qreal scale, const QString &name, const QString &tooltip,
             const QMargins &margin, bool isHat)
    : currentPixmap(image)
    , name(name)
    , tooltip(tooltip)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(true)
    , isLoaded(true)
{
}

void Image::loadImage()
{
    util::NetworkRequest req(this->getUrl());
    req.setCaller(this);
    req.setUseQuickLoadCache(true);
    req.get([lli = this](QByteArray bytes) {
        QByteArray copy = QByteArray::fromRawData(bytes.constData(), bytes.length());
        QBuffer buffer(&copy);
        buffer.open(QIODevice::ReadOnly);

        QImage image;
        QImageReader reader(&buffer);

        bool first = true;

        for (int index = 0; index < reader.imageCount(); ++index) {
            if (reader.read(&image)) {
                auto pixmap = new QPixmap(QPixmap::fromImage(image));

                if (first) {
                    first = false;
                    lli->loadedPixmap = pixmap;
                }

                chatterino::messages::Image::FrameData data;
                data.duration = std::max(20, reader.nextImageDelay());
                data.image = pixmap;

                lli->allFrames.push_back(data);
            }
        }

        if (lli->allFrames.size() > 1) {
            lli->animated = true;
        }

        lli->currentPixmap = lli->loadedPixmap;

        lli->isLoaded = true;

        singletons::EmoteManager::getInstance().incGeneration();

        postToThread([] { singletons::WindowManager::getInstance().layoutVisibleChatWidgets(); });
    });

    singletons::EmoteManager::getInstance().getGifUpdateSignal().connect([=]() {
        this->gifUpdateTimout();
    });  // For some reason when Boost signal is in thread scope and thread deletes the signal
         // doesn't work, so this is the fix.
}

void Image::gifUpdateTimout()
{
    if (this->animated) {
        this->currentFrameOffset += GIF_FRAME_LENGTH;

        while (true) {
            if (this->currentFrameOffset > this->allFrames.at(this->currentFrame).duration) {
                this->currentFrameOffset -= this->allFrames.at(this->currentFrame).duration;
                this->currentFrame = (this->currentFrame + 1) % this->allFrames.size();
            } else {
                break;
            }
        }

        this->currentPixmap = this->allFrames[this->currentFrame].image;
    }
}

const QPixmap *Image::getPixmap()
{
    if (!this->isLoading) {
        this->isLoading = true;

        this->loadImage();

        return nullptr;
    }

    if (this->isLoaded) {
        return this->currentPixmap;
    } else {
        return nullptr;
    }
}

qreal Image::getScale() const
{
    return this->scale;
}

const QString &Image::getUrl() const
{
    return this->url;
}

const QString &Image::getName() const
{
    return this->name;
}

const QString &Image::getTooltip() const
{
    return this->tooltip;
}

const QMargins &Image::getMargin() const
{
    return this->margin;
}

bool Image::isAnimated() const
{
    return this->animated;
}

bool Image::isHat() const
{
    return this->ishat;
}

int Image::getWidth() const
{
    if (this->currentPixmap == nullptr) {
        return 16;
    }

    return this->currentPixmap->width();
}

int Image::getScaledWidth() const
{
    return static_cast<int>(this->getWidth() * this->scale);
}

int Image::getHeight() const
{
    if (this->currentPixmap == nullptr) {
        return 16;
    }
    return this->currentPixmap->height();
}

int Image::getScaledHeight() const
{
    return static_cast<int>(this->getHeight() * this->scale);
}

}  // namespace messages
}  // namespace chatterino
