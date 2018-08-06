#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Log.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
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
namespace {
const QPixmap *getPixmap(const Pixmap &pixmap)
{
    if (pixmap.which() == 0)
        return boost::get<const QPixmap *>(pixmap);
    else
        return boost::get<std::unique_ptr<QPixmap>>(pixmap).get();
}

// Frames
Frames::Frames()
{
    DebugCount::increase("images");
}

Frames::Frames(std::vector<Frame> &&frames)
    : items_(std::move(frames))
{
    DebugCount::increase("images");
    if (this->animated()) DebugCount::increase("animated images");
}

Frames::~Frames()
{
    DebugCount::decrease("images");
    if (this->animated()) DebugCount::decrease("animated images");
}

void Frames::advance()
{
    this->durationOffset_ += GIF_FRAME_LENGTH;

    while (true) {
        this->index_ %= this->items_.size();
        if (this->durationOffset_ > this->items_[this->index_].duration) {
            this->durationOffset_ -= this->items_[this->index_].duration;
            this->index_ = (this->index_ + 1) % this->items_.size();
        } else {
            break;
        }
    }
}

bool Frames::animated() const
{
    return this->items_.size() > 1;
}

const QPixmap *Frames::current() const
{
    if (this->items_.size() == 0) return nullptr;
    return getPixmap(this->items_[this->index_].pixmap);
}

const QPixmap *Frames::first() const
{
    if (this->items_.size() == 0) return nullptr;
    return getPixmap(this->items_.front().pixmap);
}

// functions
std::vector<Frame> readFrames(QImageReader &reader, const Url &url)
{
    std::vector<Frame> frames;

    if (reader.imageCount() <= 0) {
        Log("Error while reading image {}: '{}'", url.string,
            reader.errorString());
        return frames;
    }

    QImage image;
    for (int index = 0; index < reader.imageCount(); ++index) {
        if (reader.read(&image)) {
            auto pixmap = std::make_unique<QPixmap>(QPixmap::fromImage(image));

            int duration = std::max(20, reader.nextImageDelay());
            frames.push_back(Frame{std::move(pixmap), duration});
        }
    }

    if (frames.size() != 0) {
        Log("Error while reading image {}: '{}'", url.string,
            reader.errorString());
    }

    return frames;
}

void queueLoadedEvent()
{
    static auto eventQueued = false;

    if (!eventQueued) {
        eventQueued = true;

        QTimer::singleShot(250, [] {
            getApp()->windows->incGeneration();
            getApp()->windows->layoutChannelViews();
            eventQueued = false;
        });
    }
}
}  // namespace

// IMAGE2
std::atomic<bool> Image::loadedEventQueued{false};

ImagePtr Image::fromUrl(const Url &url, qreal scale)
{
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
    : empty_(true)
{
}

Image::Image(const Url &url, qreal scale)
    : url_(url)
    , scale_(scale)
    , shouldLoad_(true)
{
}

Image::Image(std::unique_ptr<QPixmap> owning, qreal scale)
    : scale_(scale)
{
    std::vector<Frame> vec;
    vec.push_back(Frame{std::move(owning)});
    this->frames_ = std::move(vec);
}

Image::Image(QPixmap *nonOwning, qreal scale)
    : scale_(scale)
{
    std::vector<Frame> vec;
    vec.push_back(Frame{nonOwning});
    this->frames_ = std::move(vec);
}

const Url &Image::url() const
{
    return this->url_;
}

const QPixmap *Image::pixmap() const
{
    assertInGuiThread();

    if (this->shouldLoad_) {
        const_cast<Image *>(this)->shouldLoad_ = false;
        const_cast<Image *>(this)->load();
    }

    return this->frames_.current();
}

qreal Image::scale() const
{
    return this->scale_;
}

bool Image::empty() const
{
    return this->empty_;
}

bool Image::animated() const
{
    assertInGuiThread();

    return this->frames_.animated();
}

int Image::width() const
{
    assertInGuiThread();

    if (auto pixmap = this->frames_.first())
        return pixmap->width() * this->scale_;
    else
        return 16;
}

int Image::height() const
{
    assertInGuiThread();

    if (auto pixmap = this->frames_.first())
        return pixmap->height() * this->scale_;
    else
        return 16;
}

void Image::load()
{
    NetworkRequest req(this->url().string);
    req.setCaller(&this->object_);
    req.setUseQuickLoadCache(true);
    req.onSuccess([this, weak = weakOf(this)](auto result) -> Outcome {
        assertInGuiThread();

        auto shared = weak.lock();
        if (!shared) return Failure;

        // const cast since we are only reading from it
        QBuffer buffer(const_cast<QByteArray *>(&result.getData()));
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);

        this->frames_ = readFrames(reader, this->url());
        return Success;
    });
    req.onError([this, weak = weakOf(this)](int) {
        auto shared = weak.lock();
        if (!shared) return false;

        this->frames_ = std::vector<Frame>();

        return false;
    });

    req.execute();
}

bool Image::operator==(const Image &other) const
{
    if (this->empty() && other.empty()) return true;
    if (!this->url_.string.isEmpty() && this->url_ == other.url_) return true;
    if (this->frames_.first() == other.frames_.first()) return true;

    return false;
}

bool Image::operator!=(const Image &other) const
{
    return !this->operator==(other);
}

}  // namespace chatterino
