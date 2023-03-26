#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Benchmark.hpp"

#include <boost/functional/hash.hpp>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <functional>
#include <queue>
#include <thread>
#ifndef CHATTERINO_TEST
#    include "singletons/Emotes.hpp"
#endif
#include "singletons/helper/GifTimer.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

// Duration between each check of every Image instance
const auto IMAGE_POOL_CLEANUP_INTERVAL = std::chrono::minutes(1);
// Duration since last usage of Image pixmap before expiration of frames
const auto IMAGE_POOL_IMAGE_LIFETIME = std::chrono::minutes(10);

namespace chatterino {
namespace detail {
    // Frames
    Frames::Frames()
    {
        DebugCount::increase("images");
    }

    Frames::Frames(QVector<Frame<QPixmap>> &&frames)
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

#ifndef CHATTERINO_TEST
            this->gifTimerConnection_ =
                getApp()->emotes->gifTimer.signal.connect([this] {
                    this->advance();
                });
#endif
        }

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
#ifndef CHATTERINO_TEST
            this->durationOffset_ = std::min<int>(
                int(getApp()->emotes->gifTimer.position() % totalLength),
                60000);
#endif
        }
        this->processOffset();
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

        this->gifTimerConnection_.disconnect();
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

    boost::optional<QPixmap> Frames::current() const
    {
        if (this->items_.size() == 0)
            return boost::none;
        return this->items_[this->index_].image;
    }

    boost::optional<QPixmap> Frames::first() const
    {
        if (this->items_.size() == 0)
            return boost::none;
        return this->items_.front().image;
    }

    // functions
    QVector<Frame<QImage>> readFrames(QImageReader &reader, const Url &url)
    {
        QVector<Frame<QImage>> frames;
        frames.reserve(reader.imageCount());

        QImage image;
        for (int index = 0; index < reader.imageCount(); ++index)
        {
            if (reader.read(&image))
            {
                // It seems that browsers have special logic for fast animations.
                // This implements Chrome and Firefox's behavior which uses
                // a duration of 100 ms for any frames that specify a duration of <= 10 ms.
                // See http://webkit.org/b/36082 for more information.
                // https://github.com/SevenTV/chatterino7/issues/46#issuecomment-1010595231
                int duration = reader.nextImageDelay();
                if (duration <= 10)
                    duration = 100;
                duration = std::max(20, duration);
                frames.push_back(Frame<QImage>{std::move(image), duration});
            }
        }

        if (frames.size() == 0)
        {
            qCDebug(chatterinoImage)
                << "Error while reading image" << url.string << ": '"
                << reader.errorString() << "'";
        }

        return frames;
    }

    // parsed
    template <typename Assign>
    void assignDelayed(
        std::queue<std::pair<Assign, QVector<Frame<QPixmap>>>> &queued,
        std::mutex &mutex, std::atomic_bool &loadedEventQueued)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int i = 0;

        while (!queued.empty())
        {
            auto front = std::move(queued.front());
            queued.pop();

            // Call Assign with the vector of frames
            front.first(std::move(front.second));

            if (++i > 50)
            {
                QTimer::singleShot(3, [&] {
                    assignDelayed(queued, mutex, loadedEventQueued);
                });
                return;
            }
        }

#ifndef CHATTERINO_TEST
        getApp()->windows->forceLayoutChannelViews();
#endif
        loadedEventQueued = false;
    }

    template <typename Assign>
    auto makeConvertCallback(const QVector<Frame<QImage>> &parsed,
                             Assign assign)
    {
        static std::queue<std::pair<Assign, QVector<Frame<QPixmap>>>> queued;
        static std::mutex mutex;
        static std::atomic_bool loadedEventQueued{false};

        return [parsed, assign] {
            // convert to pixmap
            QVector<Frame<QPixmap>> frames;
            frames.reserve(parsed.size());
            std::transform(parsed.begin(), parsed.end(),
                           std::back_inserter(frames), [](auto &frame) {
                               return Frame<QPixmap>{
                                   QPixmap::fromImage(frame.image),
                                   frame.duration};
                           });

            // put into stack
            std::lock_guard<std::mutex> lock(mutex);
            queued.emplace(assign, frames);

            if (!loadedEventQueued)
            {
                loadedEventQueued = true;

                QTimer::singleShot(100, [=] {
                    assignDelayed(queued, mutex, loadedEventQueued);
                });
            }
        };
    }
}  // namespace detail

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

ImagePtr Image::fromUrl(const Url &url, qreal scale)
{
    static std::unordered_map<Url, std::weak_ptr<Image>> cache;
    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);

    auto shared = cache[url].lock();

    if (!shared)
    {
        cache[url] = shared = ImagePtr(new Image(url, scale));
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
        else
        {
            cache.erase(it);
        }
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

Image::Image(const Url &url, qreal scale)
    : url_(url)
    , scale_(scale)
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
            QVector<detail::Frame<QPixmap>>{detail::Frame<QPixmap>{pixmap, 1}});
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

    return bool(this->frames_->current());
}

boost::optional<QPixmap> Image::pixmapOrLoad() const
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
        return int(pixmap->width() * this->scale_);
    else
        return 16;
}

int Image::height() const
{
    assertInGuiThread();

    if (auto pixmap = this->frames_->first())
        return int(pixmap->height() * this->scale_);
    else
        return 16;
}

void Image::actuallyLoad()
{
    NetworkRequest(this->url().string)
        .concurrent()
        .cache()
        .onSuccess([weak = weakOf(this)](auto result) -> Outcome {
            auto shared = weak.lock();
            if (!shared)
                return Failure;

            auto data = result.getData();

            // const cast since we are only reading from it
            QBuffer buffer(const_cast<QByteArray *>(&data));
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);

            if (!reader.canRead())
            {
                qCDebug(chatterinoImage)
                    << "Error: image cant be read " << shared->url().string;
                shared->empty_ = true;
                return Failure;
            }

            const auto size = reader.size();
            if (size.isEmpty())
            {
                shared->empty_ = true;
                return Failure;
            }

            // returns 1 for non-animated formats
            if (reader.imageCount() <= 0)
            {
                qCDebug(chatterinoImage)
                    << "Error: image has less than 1 frame "
                    << shared->url().string << ": " << reader.errorString();
                shared->empty_ = true;
                return Failure;
            }

            // use "double" to prevent int overflows
            if (double(size.width()) * double(size.height()) *
                    double(reader.imageCount()) * 4.0 >
                double(Image::maxBytesRam))
            {
                qCDebug(chatterinoImage) << "image too large in RAM";

                shared->empty_ = true;
                return Failure;
            }

            auto parsed = detail::readFrames(reader, shared->url());

            postToThread(makeConvertCallback(parsed, [weak](auto &&frames) {
                if (auto shared = weak.lock())
                {
                    shared->frames_ =
                        std::make_unique<detail::Frames>(std::move(frames));
                }
            }));

            return Success;
        })
        .onError([weak = weakOf(this)](auto /*result*/) {
            auto shared = weak.lock();
            if (!shared)
                return false;

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
{
    QObject::connect(&this->freeTimer_, &QTimer::timeout, [this] {
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

    this->freeTimer_.start(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            IMAGE_POOL_CLEANUP_INTERVAL));
}

ImageExpirationPool &ImageExpirationPool::instance()
{
    static ImageExpirationPool instance;
    return instance;
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

void ImageExpirationPool::freeOld()
{
    std::lock_guard<std::mutex> lock(this->mutex_);

#    ifndef NDEBUG
    size_t numExpired = 0;
    size_t eligible = 0;
#    endif

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

#    ifndef NDEBUG
        ++eligible;
#    endif

        // Check if image has expired and, if so, expire its frame data
        auto diff = now - img->lastUsed_;
        if (diff > IMAGE_POOL_IMAGE_LIFETIME)
        {
#    ifndef NDEBUG
            ++numExpired;
#    endif
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
}

#endif

}  // namespace chatterino
