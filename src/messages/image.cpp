#include "messages/image.hpp"

#include "application.hpp"
#include "debug/log.hpp"
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

bool Image::loadedEventQueued = false;

Image::Image(const QString &url, qreal scale, const QString &name, const QString &tooltip,
             const QMargins &margin, bool isHat)
    : url(url)
    , name(name)
    , tooltip(tooltip)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
{
    util::DebugCount::increase("images");
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
    util::DebugCount::increase("images");
}

Image::~Image()
{
    util::DebugCount::decrease("images");

    if (this->isAnimated()) {
        util::DebugCount::decrease("animated images");
    }

    if (this->isLoaded) {
        util::DebugCount::decrease("loaded images");
    }
}

void Image::loadImage()
{
    util::NetworkRequest req(this->getUrl());
    req.setCaller(this);
    req.setUseQuickLoadCache(true);
    req.get([this](QByteArray bytes) -> bool {
        QByteArray copy = QByteArray::fromRawData(bytes.constData(), bytes.length());
        QBuffer buffer(&copy);
        buffer.open(QIODevice::ReadOnly);

        QImage image;
        QImageReader reader(&buffer);

        bool first = true;

        // clear stuff before loading the image again
        this->allFrames.clear();
        if (this->isAnimated()) {
            util::DebugCount::decrease("animated images");
        }
        if (this->isLoaded) {
            util::DebugCount::decrease("loaded images");
        }

        if (reader.imageCount() == -1) {
            // An error occured in the reader
            debug::Log("An error occured reading the image: '{}'", reader.errorString());
            debug::Log("Image url: {}", this->url);
            return false;
        }

        if (reader.imageCount() == 0) {
            debug::Log("Error: No images read in the buffer");
            // No images read in the buffer. maybe a cache error?
            return false;
        }

        for (int index = 0; index < reader.imageCount(); ++index) {
            if (reader.read(&image)) {
                auto pixmap = new QPixmap(QPixmap::fromImage(image));

                if (first) {
                    first = false;
                    this->loadedPixmap = pixmap;
                }

                chatterino::messages::Image::FrameData data;
                data.duration = std::max(20, reader.nextImageDelay());
                data.image = pixmap;

                this->allFrames.push_back(data);
            }
        }

        if (this->allFrames.size() != reader.imageCount()) {
            // debug::Log("Error: Wrong amount of images read");
            // One or more images failed to load from the buffer
            // return false;
        }

        if (this->allFrames.size() > 1) {
            if (!this->animated) {
                util::postToThread([this] {
                    getApp()->emotes->gifTimer.signal.connect([=]() {
                        this->gifUpdateTimout();
                    });  // For some reason when Boost signal is in
                         // thread scope and thread deletes the signal
                         // doesn't work, so this is the fix.
                });
            }

            this->animated = true;

            util::DebugCount::increase("animated images");
        }

        this->currentPixmap = this->loadedPixmap;

        this->isLoaded = true;
        util::DebugCount::increase("loaded images");

        if (!loadedEventQueued) {
            loadedEventQueued = true;

            QTimer::singleShot(500, [] {
                getApp()->windows->incGeneration();

                auto app = getApp();
                app->windows->layoutChannelViews();
                loadedEventQueued = false;
            });
        }

        return true;
    });
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

const QString &Image::getCopyString() const
{
    if (this->copyString.isEmpty()) {
        return this->name;
    }

    return this->copyString;
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
    return static_cast<int>((float)this->getWidth() * this->scale *
                            getApp()->settings->emoteScale.getValue());
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
    return static_cast<int>((float)this->getHeight() * this->scale *
                            getApp()->settings->emoteScale.getValue());
}

void Image::setCopyString(const QString &newCopyString)
{
    this->copyString = newCopyString;
}

}  // namespace messages
}  // namespace chatterino
