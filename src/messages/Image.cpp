#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Log.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <functional>
#include <thread>

namespace chatterino {

// IMAGE2
std::atomic<bool> Image::loadedEventQueued{false};

ImagePtr Image::fromUrl(const Url &url, qreal scale)
{
    // herb sutter cache
    static std::unordered_map<Url, std::weak_ptr<Image>> cache;
    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);

    auto shared = cache[url].lock();

    if (!shared) {
        cache[url] = shared = ImagePtr(new Image(url, scale));
    } else {
        Warn("same image loaded multiple times: {}", url.string);
    }

    return shared;
}

ImagePtr Image::fromOwningPixmap(std::unique_ptr<QPixmap> pixmap, qreal scale)
{
    return ImagePtr(new Image(std::move(pixmap), scale));
}

ImagePtr Image::fromNonOwningPixmap(QPixmap *pixmap, qreal scale)
{
    return ImagePtr(new Image(pixmap, scale));
}

ImagePtr Image::getEmpty()
{
    static auto empty = ImagePtr(new Image);
    return empty;
}

Image::Image()
{
    this->isLoaded_ = true;
    this->isNull_ = true;
}

Image::Image(const Url &url, qreal scale)
{
    this->url_ = url;
    this->scale_ = scale;

    if (url.string.isEmpty()) {
        this->isLoaded_ = true;
        this->isNull_ = true;
    }
}

Image::Image(std::unique_ptr<QPixmap> owning, qreal scale)
{
    this->frames_.push_back(Frame(std::move(owning)));
    this->scale_ = scale;
    this->isLoaded_ = true;
    this->currentFramePixmap_ = this->frames_.front().getPixmap();
}

Image::Image(QPixmap *nonOwning, qreal scale)
{
    this->frames_.push_back(Frame(nonOwning));
    this->scale_ = scale;
    this->isLoaded_ = true;
    this->currentFramePixmap_ = this->frames_.front().getPixmap();
}

const Url &Image::getUrl() const
{
    return this->url_;
}

NullablePtr<const QPixmap> Image::getPixmap() const
{
    assertInGuiThread();

    if (!this->isLoaded_) {
        const_cast<Image *>(this)->load();
    }

    return this->currentFramePixmap_;
}

qreal Image::getScale() const
{
    return this->scale_;
}

bool Image::isAnimated() const
{
    return this->isAnimated_;
}

int Image::getWidth() const
{
    if (!this->isLoaded_) return 16;

    return this->frames_.front().getPixmap()->width() * this->scale_;
}

int Image::getHeight() const
{
    if (!this->isLoaded_) return 16;

    return this->frames_.front().getPixmap()->height() * this->scale_;
}

bool Image::isLoaded() const
{
    return this->isLoaded_;
}

bool Image::isValid() const
{
    return !this->isNull_;
}

bool Image::isNull() const
{
    return this->isNull_;
}

void Image::load()
{
    // decrease debug count
    if (this->isAnimated_) {
        DebugCount::decrease("animated images");
    }
    if (this->isLoaded_) {
        DebugCount::decrease("loaded images");
    }

    this->isLoaded_ = false;
    this->isLoading_ = true;
    this->frames_.clear();

    NetworkRequest req(this->getUrl().string);
    req.setCaller(&this->object_);
    req.setUseQuickLoadCache(true);
    req.onSuccess([this, weak = weakOf(this)](auto result) -> Outcome {
        auto shared = weak.lock();
        if (!shared) return Failure;

        auto &bytes = result.getData();
        QByteArray copy = QByteArray::fromRawData(bytes.constData(), bytes.length());

        return this->parse(result.getData());
    });

    req.execute();
}

Outcome Image::parse(const QByteArray &data)
{
    // const cast since we are only reading from it
    QBuffer buffer(const_cast<QByteArray *>(&data));
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);

    return this->setFrames(this->readFrames(reader));
}

std::vector<Image::Frame> Image::readFrames(QImageReader &reader)
{
    std::vector<Frame> frames;

    if (reader.imageCount() <= 0) {
        Log("Error while reading image {}: '{}'", this->url_.string, reader.errorString());
        return frames;
    }

    QImage image;
    for (int index = 0; index < reader.imageCount(); ++index) {
        if (reader.read(&image)) {
            auto pixmap = new QPixmap(QPixmap::fromImage(image));

            int duration = std::max(20, reader.nextImageDelay());
            frames.push_back(Image::Frame(pixmap, duration));
        }
    }

    if (frames.size() != 0) {
        Log("Error while reading image {}: '{}'", this->url_.string, reader.errorString());
    }

    return frames;
}

Outcome Image::setFrames(std::vector<Frame> frames)
{
    std::lock_guard<std::mutex> lock(this->framesMutex_);

    if (frames.size() > 0) {
        this->currentFramePixmap_ = frames.front().getPixmap();

        if (frames.size() > 1) {
            if (!this->isAnimated_) {
                getApp()->emotes->gifTimer.signal.connect([=]() { this->updateAnimation(); });
            }

            this->isAnimated_ = true;
            DebugCount::increase("animated images");
        }

        this->isLoaded_ = true;
        DebugCount::increase("loaded images");

        return Success;
    }

    this->frames_ = std::move(frames);
    this->queueLoadedEvent();

    return Failure;
}

void Image::queueLoadedEvent()
{
    if (!loadedEventQueued) {
        loadedEventQueued = true;

        QTimer::singleShot(250, [] {
            getApp()->windows->incGeneration();
            getApp()->windows->layoutChannelViews();
            loadedEventQueued = false;
        });
    }
}

void Image::updateAnimation()
{
    if (this->isAnimated_) {
        std::lock_guard<std::mutex> lock(this->framesMutex_);

        this->currentFrameOffset_ += GIF_FRAME_LENGTH;

        while (true) {
            this->currentFrameIndex_ %= this->frames_.size();
            if (this->currentFrameOffset_ > this->frames_[this->currentFrameIndex_].getDuration()) {
                this->currentFrameOffset_ -= this->frames_[this->currentFrameIndex_].getDuration();
                this->currentFrameIndex_ = (this->currentFrameIndex_ + 1) % this->frames_.size();
            } else {
                break;
            }
        }

        this->currentFramePixmap_ = this->frames_[this->currentFrameIndex_].getPixmap();
    }
}

bool Image::operator==(const Image &other) const
{
    if (this->isNull() && other.isNull()) {
        return true;
    }

    if (!this->url_.string.isEmpty() && this->url_ == other.url_) {
        return true;
    }

    assert(this->frames_.size() == 1);
    assert(other.frames_.size() == 1);

    if (this->currentFramePixmap_ == other.currentFramePixmap_) {
        return true;
    }

    return false;
}

bool Image::operator!=(const Image &other) const
{
    return !this->operator==(other);
}

// FRAME
Image::Frame::Frame(QPixmap *nonOwning, int duration)
    : nonOwning_(nonOwning)
    , duration_(duration)
{
}

Image::Frame::Frame(std::unique_ptr<QPixmap> nonOwning, int duration)
    : owning_(std::move(nonOwning))
    , duration_(duration)
{
}

int Image::Frame::getDuration() const
{
    return this->duration_;
}

QPixmap *Image::Frame::getPixmap() const
{
    if (this->nonOwning_) return this->nonOwning_;

    return this->owning_.get();
}

}  // namespace chatterino
