// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/emotes/EmoteController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Benchmark.hpp"
#include "singletons/helper/GifTimer.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

#include <boost/functional/hash.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <atomic>
#include <numeric>
#include <utility>

// Duration between each check of every Image instance
const auto IMAGE_POOL_CLEANUP_INTERVAL = std::chrono::minutes(1);
// Duration since last usage of Image pixmap before expiration of frames
const auto IMAGE_POOL_IMAGE_LIFETIME = std::chrono::minutes(10);

// Stop decoding dynamic frames shortly after they are no longer painted.
const auto DYNAMIC_FRAMES_PAUSE_AFTER = std::chrono::seconds(1);
// Dynamic frames retain the encoded file for playback, so limit it separately
// from the decoded frame size.
constexpr int MAX_DYNAMIC_IMAGE_DATA_BYTES = 20 * 1024 * 1024;

namespace chatterino::detail {

struct Frames::Storage {
    Storage() = default;
    virtual ~Storage() = default;
    Storage(const Storage &) = delete;
    Storage &operator=(const Storage &) = delete;
    Storage(Storage &&) = delete;
    Storage &operator=(Storage &&) = delete;

    virtual int64_t memoryUsage() const = 0;
    virtual bool empty() const = 0;
    virtual bool animated() const = 0;
    virtual void start(GIFTimer * /*unused*/)
    {
    }
    virtual void onPaint(std::chrono::steady_clock::time_point /*paintTime*/)
    {
    }
    virtual std::optional<QPixmap> current() const = 0;
    virtual std::optional<QSize> frameSize() const = 0;

    pajlada::Signals::ScopedConnection gifTimerConnection;
};

/// Stores all decoded frames in memory.
struct Frames::CachedFrames : Storage {
    CachedFrames() = default;
    explicit CachedFrames(QList<Frame> frames)
        : items(std::move(frames))
    {
    }
    ~CachedFrames() override = default;
    CachedFrames(const CachedFrames &) = delete;
    CachedFrames &operator=(const CachedFrames &) = delete;
    CachedFrames(CachedFrames &&) = delete;
    CachedFrames &operator=(CachedFrames &&) = delete;

    int64_t memoryUsage() const override
    {
        int64_t usage = 0;
        for (const auto &frame : this->items)
        {
            auto sz = frame.image.size();
            auto area = sz.width() * sz.height();
            auto memory = area * frame.image.depth() / 8;
            usage += memory;
        }
        return usage;
    }

    bool empty() const override
    {
        return this->items.empty();
    }

    bool animated() const override
    {
        return this->items.size() > 1;
    }

    void start(GIFTimer *timer) override
    {
        if (!this->animated())
        {
            return;
        }

        this->gifTimerConnection = timer->signal.connect([this] {
            this->advance();
        });

        const auto totalLength =
            std::accumulate(this->items.begin(), this->items.end(), 0UL,
                            [](auto init, auto &&frame) {
                                return init + frame.duration;
                            });
        if (totalLength == 0)
        {
            this->durationOffset = 0;
        }
        else
        {
            this->durationOffset =
                std::min<int>(int(timer->position() % totalLength), 60000);
        }
        this->processOffset();
    }

    void advance()
    {
        this->durationOffset += GIF_FRAME_LENGTH;
        this->processOffset();
    }

    void processOffset()
    {
        if (this->items.isEmpty())
        {
            return;
        }

        while (true)
        {
            this->index %= this->items.size();
            if (this->durationOffset > this->items.at(this->index).duration)
            {
                this->durationOffset -= this->items.at(this->index).duration;
                this->index = (this->index + 1) % this->items.size();
            }
            else
            {
                break;
            }
        }
    }

    std::optional<QPixmap> current() const override
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return this->items[this->index].image;
    }

    std::optional<QSize> frameSize() const override
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return this->items.front().image.size();
    }

    QList<Frame> items;
    QList<Frame>::size_type index{0};
    int durationOffset{0};
};

struct Frames::DynamicFrames : Storage {
    explicit DynamicFrames(QByteArray bytes)
        : data(std::move(bytes))
        , buffer(&this->data)
    {
        this->buffer.open(QIODevice::ReadOnly);
        this->reader = std::make_unique<QImageReader>(&this->buffer);
    }

    ~DynamicFrames() override = default;
    DynamicFrames(const DynamicFrames &) = delete;
    DynamicFrames &operator=(const DynamicFrames &) = delete;
    DynamicFrames(DynamicFrames &&) = delete;
    DynamicFrames &operator=(DynamicFrames &&) = delete;

    bool readNext()
    {
        auto image = this->reader->read();
        if (image.isNull())
        {
            return false;
        }
        this->pixmap = QPixmap::fromImage(std::move(image));
        return !this->pixmap.isNull();
    }

    int frameDelay() const
    {
        if (!this->reader->supportsAnimation())
        {
            return 1000;
        }
        return this->reader->nextImageDelay();
    }

    bool decodeNext()
    {
        if (this->frameIndex + 1 < this->frameCount)
        {
            if (!this->readNext())
            {
                return false;
            }
            ++this->frameIndex;
            return true;
        }
        if (this->remainingLoops == 0)
        {
            return false;
        }
        if (this->remainingLoops > 0)
        {
            --this->remainingLoops;
        }
        this->reader.reset();
        this->buffer.seek(0);
        this->reader = std::make_unique<QImageReader>(&this->buffer);
        if (!this->readNext())
        {
            return false;
        }
        this->frameIndex = 0;
        return true;
    }

    int64_t memoryUsage() const override
    {
        return this->data.size() +
               (int64_t(this->pixmap.width()) * this->pixmap.height() *
                this->pixmap.depth() / 8);
    }

    bool empty() const override
    {
        return this->pixmap.isNull();
    }

    bool animated() const override
    {
        return this->frameCount > 1;
    }

    void start(GIFTimer *timer) override
    {
        if (this->animated())
        {
            this->gifTimerConnection = timer->signal.connect([this] {
                this->advance();
            });
        }
    }

    void advance()
    {
        if (this->finished || this->paused)
        {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        if (now - this->lastUsed > DYNAMIC_FRAMES_PAUSE_AFTER)
        {
            this->paused = true;
            return;
        }
        if (now < this->nextFrame)
        {
            return;
        }

        // Avoid decoding a backlog of frames after a stall.
        this->nextFrame = std::max(
            this->nextFrame, now - std::chrono::milliseconds(GIF_FRAME_LENGTH));

        while (this->nextFrame <= now)
        {
            if (!this->decodeNext())
            {
                this->finished = true;
                this->gifTimerConnection = pajlada::Signals::ScopedConnection{};
                return;
            }
            const auto delay = this->frameDelay();
            this->nextFrame += std::chrono::milliseconds(delay);
            if (delay == 0)
            {
                this->nextFrame = std::chrono::steady_clock::now();
                break;
            }
        }
    }

    void onPaint(std::chrono::steady_clock::time_point paintTime) override
    {
        if (!this->animated() || this->finished)
        {
            return;
        }
        if (this->paused ||
            paintTime - this->lastUsed > DYNAMIC_FRAMES_PAUSE_AFTER)
        {
            this->nextFrame =
                paintTime + std::chrono::milliseconds(this->frameDelay());
            this->paused = false;
        }
        this->lastUsed = paintTime;
    }

    std::optional<QPixmap> current() const override
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return this->pixmap;
    }

    std::optional<QSize> frameSize() const override
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return this->pixmap.size();
    }

    // This order is important for destruction; reader uses buffer, buffer uses data.
    QByteArray data;
    QBuffer buffer;
    std::unique_ptr<QImageReader> reader;
    QPixmap pixmap;
    std::chrono::steady_clock::time_point lastUsed;
    std::chrono::steady_clock::time_point nextFrame;
    int frameCount = 0;
    int frameIndex = 0;
    int remainingLoops = 0;
    bool paused = true;
    bool finished = false;
};

Frames::Frames()
    : storage_(std::make_unique<CachedFrames>())
{
    DebugCount::increase(DebugObject::Image);
}

Frames::Frames(QList<Frame> &&frames)
    : storage_(std::make_unique<CachedFrames>(std::move(frames)))
{
    assertInGuiThread();
    auto *app = tryGetApp();
    if (app == nullptr)
    {
        qCDebug(chatterinoImage)
            << "Frames constructor called while app is shutting down";
        return;
    }

    DebugCount::increase(DebugObject::Image);
    if (!this->empty())
    {
        DebugCount::increase(DebugObject::LoadedImage);
    }

    if (this->animated())
    {
        DebugCount::increase(DebugObject::AnimatedImage);

        this->storage_->start(app->getEmotes()->getGIFTimer());
    }

    DebugCount::increase(DebugObject::BytesImageCurrent, this->memoryUsage());
    DebugCount::increase(DebugObject::BytesImageLoaded, this->memoryUsage());
}

Frames::Frames(QByteArray data)
    : Frames()
{
    assertInGuiThread();
    auto frames = std::make_unique<DynamicFrames>(std::move(data));
    if (!frames->reader->canRead() || !frames->readNext())
    {
        qCDebug(chatterinoImage)
            << "Error reading image:" << frames->reader->errorString();
        return;
    }

    frames->frameCount = frames->reader->imageCount();
    frames->remainingLoops = frames->reader->loopCount();
    this->storage_ = std::move(frames);
    DebugCount::increase(DebugObject::LoadedImage);
    if (this->animated())
    {
        DebugCount::increase(DebugObject::AnimatedImage);
        if (auto *app = tryGetApp())
        {
            this->storage_->start(app->getEmotes()->getGIFTimer());
        }
    }
    DebugCount::increase(DebugObject::BytesImageCurrent, this->memoryUsage());
    DebugCount::increase(DebugObject::BytesImageLoaded, this->memoryUsage());
}

Frames::~Frames()
{
    assertInGuiThread();
    DebugCount::decrease(DebugObject::Image);
    if (!this->empty())
    {
        DebugCount::decrease(DebugObject::LoadedImage);
    }

    if (this->animated())
    {
        DebugCount::decrease(DebugObject::AnimatedImage);
    }
    DebugCount::decrease(DebugObject::BytesImageCurrent, this->memoryUsage());
    DebugCount::increase(DebugObject::BytesImageUnloaded, this->memoryUsage());
}

int64_t Frames::memoryUsage() const
{
    return this->storage_->memoryUsage();
}

void Frames::clear()
{
    assertInGuiThread();
    if (!this->empty())
    {
        DebugCount::decrease(DebugObject::LoadedImage);
    }
    DebugCount::decrease(DebugObject::BytesImageCurrent, this->memoryUsage());
    DebugCount::increase(DebugObject::BytesImageUnloaded, this->memoryUsage());

    if (this->animated())
    {
        DebugCount::decrease(DebugObject::AnimatedImage);
    }
    this->storage_ = std::make_unique<CachedFrames>();
}

void Frames::onPaint(std::chrono::steady_clock::time_point paintTime)
{
    this->storage_->onPaint(paintTime);
}

bool Frames::empty() const
{
    return this->storage_->empty();
}

bool Frames::animated() const
{
    return this->storage_->animated();
}

std::optional<QPixmap> Frames::current() const
{
    return this->storage_->current();
}

std::optional<QSize> Frames::frameSize() const
{
    return this->storage_->frameSize();
}

QList<Frame> readFrames(QImageReader &reader, const Url &url)
{
    QList<Frame> frames;
    frames.reserve(reader.imageCount());

    for (int index = 0; index < reader.imageCount(); ++index)
    {
        auto pixmap = QPixmap::fromImageReader(&reader);
        if (!pixmap.isNull())
        {
            // It seems that browsers have special logic for fast animations.
            // This implements Chrome and Firefox's behavior which uses
            // a duration of 100 ms for any frames that specify a duration of <= 10 ms.
            // See http://webkit.org/b/36082 for more information.
            // https://github.com/SevenTV/chatterino7/issues/46#issuecomment-1010595231
            int duration = reader.nextImageDelay();
            if (duration <= 10)
            {
                duration = 100;
            }
            duration = std::max(20, duration);
            frames.append(Frame{
                .image = std::move(pixmap),
                .duration = duration,
            });
        }
    }

    if (frames.empty())
    {
        qCDebug(chatterinoImage) << "Error while reading image" << url.string
                                 << ": '" << reader.errorString() << "'";
    }

    return frames;
}

void assignFrames(std::weak_ptr<Image> weak, QList<Frame> parsed,
                  QByteArray dynamicData)
{
    static bool isPushQueued;

    auto cb = [parsed = std::move(parsed), dynamicData = std::move(dynamicData),
               weak = std::move(weak)]() mutable {
        auto shared = weak.lock();
        if (!shared)
        {
            return;
        }
        if (dynamicData.isEmpty())
        {
            shared->frames_ =
                std::make_unique<detail::Frames>(std::move(parsed));
        }
        else
        {
            shared->frames_ =
                std::make_unique<detail::Frames>(std::move(dynamicData));
            shared->empty_ = shared->frames_->empty();
        }

        // Avoid too many layouts in one event-loop iteration
        //
        // This callback is called for every image, so there might be multiple
        // callbacks queued on the event-loop in this iteration, but we only
        // want to generate one invalidation.
        if (!isPushQueued)
        {
            isPushQueued = true;
            // We don't use postToThread here, because that would run immediately.
            // We explicitly want to queue a callback after the current ones.
            QMetaObject::invokeMethod(
                qApp,
                [] {
                    isPushQueued = false;
                    auto *app = tryGetApp();
                    if (app != nullptr)
                    {
                        app->getWindows()->forceLayoutChannelViews();
                    }
                },
                Qt::QueuedConnection);
        }
    };

    postToGuiThread(cb);
}

}  // namespace chatterino::detail

namespace chatterino {

// IMAGE2
Image::~Image()
{
#ifndef DISABLE_IMAGE_EXPIRATION_POOL
    ImageExpirationPool::instance().removeImagePtr(this);
#endif

    if (this->empty_ && !this->frames_)
    {
        // No data in this image, don't bother trying to release it
        // The reason we do this check is that we keep a few (or one) static empty image around that are deconstructed at the end of the programs lifecycle, and we want to prevent the isGuiThread call to be called after the QApplication has been exited
        return;
    }

    if (isAppAboutToQuit())
    {
        if (this->frames_)
        {
            std::ignore = this->frames_.release();
        }
        return;
    }

    // Ensure the destructor for our frames is called in the GUI thread
    // If the Image destructor is called outside of the GUI thread, move the
    // ownership of the frames to the GUI thread, otherwise the frames will be
    // destructed as part as we go out of scope
    if (!isGuiThread())
    {
        postToThread([frames = this->frames_.release()]() {
            delete frames;
        });
    }
}

ImagePtr Image::fromUrl(const Url &url, qreal scale, QSize expectedSize)
{
    static std::unordered_map<Url, std::weak_ptr<Image>> cache;
    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);

    auto shared = cache[url].lock();

    if (!shared)
    {
        cache[url] = shared = ImagePtr(new Image(url, scale, expectedSize));
    }

    return shared;
}

ImagePtr Image::fromUrlWithDynamicFrames(const Url &url, qreal scale,
                                         QSize expectedSize)
{
    // Cache images with dynamic frames separately.
    static std::unordered_map<Url, std::weak_ptr<Image>> cache;
    static std::mutex mutex;

    std::scoped_lock lock(mutex);
    auto shared = cache[url].lock();
    if (!shared)
    {
        cache[url] = shared =
            ImagePtr(new Image(url, scale, expectedSize, true));
    }
    return shared;
}

ImagePtr Image::fromResourcePixmap(const QPixmap &pixmap, qreal scale)
{
    using key_t = std::pair<const QPixmap *, qreal>;
    static std::unordered_map<key_t, std::weak_ptr<Image>, boost::hash<key_t>>
        cache;
    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find({&pixmap, scale});
    if (it != cache.end())
    {
        auto shared = it->second.lock();
        if (shared)
        {
            return shared;
        }

        cache.erase(it);
    }

    auto newImage = ImagePtr(new Image(scale));

    newImage->setPixmap(pixmap);

    // store in cache
    cache.insert({{&pixmap, scale}, std::weak_ptr<Image>(newImage)});

    return newImage;
}

ImagePtr Image::getEmpty()
{
    static auto empty = ImagePtr(new Image);
    return empty;
}

ImagePtr getEmptyImagePtr()
{
    return Image::getEmpty();
}

Image::Image()
    : empty_(true)
{
}

Image::Image(Url url, qreal scale, QSize expectedSize, bool useDynamicFrames)
    : url_(std::move(url))
    , scale_(scale)
    , expectedSize_(expectedSize.isValid() ? expectedSize
                                           : (QSize(16, 16) * scale))
    , useDynamicFrames_(useDynamicFrames)
    , shouldLoad_(true)
    , frames_(std::make_unique<detail::Frames>())
{
}

Image::Image(qreal scale)
    : scale_(scale)
    , frames_(std::make_unique<detail::Frames>())
{
}

void Image::setPixmap(const QPixmap &pixmap)
{
    auto setFrames = [shared = this->shared_from_this(), pixmap]() {
        shared->frames_ = std::make_unique<detail::Frames>(
            QList<detail::Frame>{detail::Frame{pixmap, 1}});
    };

    if (isGuiThread())
    {
        setFrames();
    }
    else
    {
        postToThread(setFrames);
    }
}

const Url &Image::url() const
{
    return this->url_;
}

bool Image::loaded() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return false;
    }

    return this->frames_->current().has_value();
}

std::optional<QPixmap> Image::pixmapOrLoad() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return std::nullopt;
    }

    // Mark the image as just used.
    // Any time this Image is painted, this method is invoked.
    // See src/messages/layouts/MessageLayoutElement.cpp ImageLayoutElement::paint, for example.
    this->lastUsed_ = std::chrono::steady_clock::now();

    this->load();
    this->frames_->onPaint(this->lastUsed_);

    return this->frames_->current();
}

void Image::load() const
{
    assertInGuiThread();

    if (this->shouldLoad_)
    {
        Image *this2 = const_cast<Image *>(this);
        this2->shouldLoad_ = false;
        this2->actuallyLoad();
#ifndef DISABLE_IMAGE_EXPIRATION_POOL
        ImageExpirationPool::instance().addImagePtr(this2->shared_from_this());
#endif
    }
}

qreal Image::scale() const
{
    return this->scale_;
}

bool Image::isEmpty() const
{
    return this->empty_;
}

bool Image::animated() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return false;
    }

    return this->frames_->animated();
}

int Image::width() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return 0;
    }

    if (auto size = this->frames_->frameSize())
    {
        return static_cast<int>(size->width() * this->scale_);
    }

    // No frames loaded, use the expected size
    return static_cast<int>(this->expectedSize_.width() * this->scale_);
}

int Image::height() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return 0;
    }

    if (auto size = this->frames_->frameSize())
    {
        return static_cast<int>(size->height() * this->scale_);
    }

    // No frames loaded, use the expected size
    return static_cast<int>(this->expectedSize_.height() * this->scale_);
}

QSizeF Image::size() const
{
    assertInGuiThread();

    if (!this->frames_)
    {
        return {0, 0};
    }

    if (auto size = this->frames_->frameSize())
    {
        return size->toSizeF() * this->scale_;
    }

    // No frames loaded, use the expected size
    return this->expectedSize_.toSizeF() * this->scale_;
}

void Image::actuallyLoad()
{
    auto weak = weakOf(this);
    NetworkRequest(this->url().string)
        .concurrent()
        .cache()
        .onSuccess([weak](auto result) {
            auto shared = weak.lock();
            if (!shared)
            {
                return;
            }

            assert(!isAppAboutToQuit());

            auto data = result.getData();
            if (shared->useDynamicFrames_ &&
                data.size() > MAX_DYNAMIC_IMAGE_DATA_BYTES)
            {
                qCDebug(chatterinoImage)
                    << "dynamic image data too large" << shared->url().string;
                shared->empty_ = true;
                return;
            }

            QBuffer buffer;
            buffer.setData(data);
            QImageReader reader(&buffer);

            if (!reader.canRead())
            {
                qCDebug(chatterinoImage)
                    << "Error: image cant be read " << shared->url().string;
                shared->empty_ = true;
                return;
            }

            const auto size = reader.size();
            if (size.isEmpty())
            {
                shared->empty_ = true;
                return;
            }

            // returns 1 for non-animated formats
            if (reader.imageCount() <= 0)
            {
                qCDebug(chatterinoImage)
                    << "Error: image has less than 1 frame "
                    << shared->url().string << ": " << reader.errorString();
                shared->empty_ = true;
                return;
            }

            if (shared->useDynamicFrames_)
            {
                // Check the memory needed for one decoded frame.
                if (double(size.width()) * double(size.height()) * 4.0 >
                    double(Image::maxBytesRam))
                {
                    qCDebug(chatterinoImage) << "dynamic image frame too large"
                                             << shared->url().string;
                    shared->empty_ = true;
                    return;
                }
                detail::assignFrames(shared, {}, std::move(data));
                return;
            }

            // use "double" to prevent int overflows
            if (double(size.width()) * double(size.height()) *
                    double(reader.imageCount()) * 4.0 >
                double(Image::maxBytesRam))
            {
                qCDebug(chatterinoImage) << "image too large in RAM";

                shared->empty_ = true;
                return;
            }

            auto parsed = detail::readFrames(reader, shared->url());

            assignFrames(shared, parsed);
        })
        .onError([weak](auto /*result*/) {
            auto shared = weak.lock();
            if (!shared)
            {
                return false;
            }

            // fourtf: is this the right thing to do?
            shared->empty_ = true;

            return true;
        })
        .execute();
}

void Image::expireFrames()
{
    assertInGuiThread();
    if (!this->frames_)
    {
        return;
    }

    this->frames_->clear();
    this->shouldLoad_ = true;  // Mark as needing load again
}

#ifndef DISABLE_IMAGE_EXPIRATION_POOL

ImageExpirationPool::ImageExpirationPool()
    : freeTimer_(new QTimer)
{
    QObject::connect(this->freeTimer_, &QTimer::timeout, [this] {
        if (isGuiThread())
        {
            this->freeOld();
        }
        else
        {
            postToThread([this] {
                this->freeOld();
            });
        }
    });

    this->freeTimer_->start(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            IMAGE_POOL_CLEANUP_INTERVAL));
}

ImageExpirationPool &ImageExpirationPool::instance()
{
    static auto *instance = new ImageExpirationPool;
    return *instance;
}

void ImageExpirationPool::addImagePtr(ImagePtr imgPtr)
{
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->allImages_.emplace(imgPtr.get(), std::weak_ptr<Image>(imgPtr));
}

void ImageExpirationPool::removeImagePtr(Image *rawPtr)
{
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->allImages_.erase(rawPtr);
}

void ImageExpirationPool::freeAll()
{
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        for (auto it = this->allImages_.begin(); it != this->allImages_.end();)
        {
            auto img = it->second.lock();
            img->expireFrames();
            it = this->allImages_.erase(it);
        }
    }
    this->freeOld();
}

void ImageExpirationPool::freeOld()
{
    std::lock_guard<std::mutex> lock(this->mutex_);

    size_t numExpired = 0;
    size_t eligible = 0;

    auto now = std::chrono::steady_clock::now();
    for (auto it = this->allImages_.begin(); it != this->allImages_.end();)
    {
        auto img = it->second.lock();
        if (!img)
        {
            // This can only really happen from a race condition because ~Image
            // should remove itself from the ImageExpirationPool automatically.
            it = this->allImages_.erase(it);
            continue;
        }

        if (img->frames_->empty())
        {
            // No frame data, nothing to do
            ++it;
            continue;
        }

        ++eligible;

        // Check if image has expired and, if so, expire its frame data
        auto diff = now - img->lastUsed_;
        if (diff > IMAGE_POOL_IMAGE_LIFETIME)
        {
            ++numExpired;
            img->expireFrames();
            // erase without mutex locking issue
            it = this->allImages_.erase(it);
            continue;
        }

        ++it;
    }

#    ifndef NDEBUG
    qCDebug(chatterinoImage) << "freed frame data for" << numExpired << "/"
                             << eligible << "eligible images";
#    endif
    DebugCount::set(DebugObject::LastImageGcExpired, numExpired);
    DebugCount::set(DebugObject::LastImageGcEligible, eligible);
    DebugCount::set(DebugObject::LastImageGcLeft, this->allImages_.size());
}

#endif

}  // namespace chatterino
