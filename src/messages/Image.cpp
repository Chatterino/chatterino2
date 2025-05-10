#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Benchmark.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/helper/GifTimer.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

#include <boost/functional/hash.hpp>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <atomic>

// Duration between each check of every Image instance
const auto IMAGE_POOL_CLEANUP_INTERVAL = std::chrono::minutes(1);
// Duration since last usage of Image pixmap before expiration of frames
const auto IMAGE_POOL_IMAGE_LIFETIME = std::chrono::minutes(10);

namespace chatterino::detail {

Frames::Frames()
{
    DebugCount::increase("images");
}

Frames::Frames(QList<Frame> &&frames)
    : items_(std::move(frames))
{
    assertInGuiThread();
    DebugCount::increase("images");
    if (!this->empty())
    {
        DebugCount::increase("loaded images");
    }

    if (this->animated())
    {
        DebugCount::increase("animated images");

        this->gifTimerConnection_ =
            getApp()->getEmotes()->getGIFTimer().signal.connect([this] {
                this->advance();
            });

        auto totalLength =
            std::accumulate(this->items_.begin(), this->items_.end(), 0UL,
                            [](auto init, auto &&frame) {
                                return init + frame.duration;
                            });

        if (totalLength == 0)
        {
            this->durationOffset_ = 0;
        }
        else
        {
            this->durationOffset_ = std::min<int>(
                int(getApp()->getEmotes()->getGIFTimer().position() %
                    totalLength),
                60000);
        }
        this->processOffset();
    }

    DebugCount::increase("image bytes", this->memoryUsage());
    DebugCount::increase("image bytes (ever loaded)", this->memoryUsage());
}

Frames::~Frames()
{
    assertInGuiThread();
    DebugCount::decrease("images");
    if (!this->empty())
    {
        DebugCount::decrease("loaded images");
    }

    if (this->animated())
    {
        DebugCount::decrease("animated images");
    }
    DebugCount::decrease("image bytes", this->memoryUsage());
    DebugCount::increase("image bytes (ever unloaded)", this->memoryUsage());

    this->gifTimerConnection_.disconnect();
}

int64_t Frames::memoryUsage() const
{
    int64_t usage = 0;
    for (const auto &frame : this->items_)
    {
        auto sz = frame.image.size();
        auto area = sz.width() * sz.height();
        auto memory = area * frame.image.depth() / 8;

        usage += memory;
    }
    return usage;
}

void Frames::advance()
{
    this->durationOffset_ += GIF_FRAME_LENGTH;
    this->processOffset();
}

void Frames::processOffset()
{
    if (this->items_.isEmpty())
    {
        return;
    }

    while (true)
    {
        this->index_ %= this->items_.size();

        if (this->durationOffset_ > this->items_[this->index_].duration)
        {
            this->durationOffset_ -= this->items_[this->index_].duration;
            this->index_ = (this->index_ + 1) % this->items_.size();
        }
        else
        {
            break;
        }
    }
}

void Frames::clear()
{
    assertInGuiThread();
    if (!this->empty())
    {
        DebugCount::decrease("loaded images");
    }
    DebugCount::decrease("image bytes", this->memoryUsage());
    DebugCount::increase("image bytes (ever unloaded)", this->memoryUsage());

    this->items_.clear();
    this->index_ = 0;
    this->durationOffset_ = 0;
    this->gifTimerConnection_.disconnect();
}

bool Frames::empty() const
{
    return this->items_.empty();
}

bool Frames::animated() const
{
    return this->items_.size() > 1;
}

std::optional<QPixmap> Frames::current() const
{
    if (this->items_.empty())
    {
        return std::nullopt;
    }

    return this->items_[this->index_].image;
}

std::optional<QPixmap> Frames::first() const
{
    if (this->items_.empty())
    {
        return std::nullopt;
    }

    return this->items_.front().image;
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

void assignFrames(std::weak_ptr<Image> weak, QList<Frame> parsed)
{
    static bool isPushQueued;

    auto cb = [parsed = std::move(parsed), weak = std::move(weak)]() mutable {
        auto shared = weak.lock();
        if (!shared)
        {
            return;
        }
        shared->frames_ = std::make_unique<detail::Frames>(std::move(parsed));

        // Avoid too many layouts in one event-loop iteration
        //
        // This callback is called for every image, so there might be multiple
        // callbacks queued on the event-loop in this iteration, but we only
        // want to generate one invalidation.
        if (!isPushQueued)
        {
            isPushQueued = true;
            postToThread([] {
                isPushQueued = false;
                auto *app = tryGetApp();
                if (app != nullptr)
                {
                    app->getWindows()->forceLayoutChannelViews();
                }
            });
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

Image::Image(const Url &url, qreal scale, QSize expectedSize)
    : url_(url)
    , scale_(scale)
    , expectedSize_(expectedSize.isValid() ? expectedSize
                                           : (QSize(16, 16) * scale))
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

    return this->frames_->current().has_value();
}

std::optional<QPixmap> Image::pixmapOrLoad() const
{
    assertInGuiThread();

    // Mark the image as just used.
    // Any time this Image is painted, this method is invoked.
    // See src/messages/layouts/MessageLayoutElement.cpp ImageLayoutElement::paint, for example.
    this->lastUsed_ = std::chrono::steady_clock::now();

    this->load();

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

    return this->frames_->animated();
}

int Image::width() const
{
    assertInGuiThread();

    if (auto pixmap = this->frames_->first())
    {
        return static_cast<int>(pixmap->width() * this->scale_);
    }

    // No frames loaded, use the expected size
    return static_cast<int>(this->expectedSize_.width() * this->scale_);
}

int Image::height() const
{
    assertInGuiThread();

    if (auto pixmap = this->frames_->first())
    {
        return static_cast<int>(pixmap->height() * this->scale_);
    }

    // No frames loaded, use the expected size
    return static_cast<int>(this->expectedSize_.height() * this->scale_);
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

            QBuffer buffer;
            buffer.setData(result.getData());
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

    // configure all debug counts used by images
    DebugCount::configure("image bytes", DebugCount::Flag::DataSize);
    DebugCount::configure("image bytes (ever loaded)",
                          DebugCount::Flag::DataSize);
    DebugCount::configure("image bytes (ever unloaded)",
                          DebugCount::Flag::DataSize);
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
    DebugCount::set("last image gc: expired", numExpired);
    DebugCount::set("last image gc: eligible", eligible);
    DebugCount::set("last image gc: left after gc", this->allImages_.size());
}

#endif

}  // namespace chatterino
