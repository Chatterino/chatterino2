#include "messages/Image.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <functional>
#include <thread>

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Benchmark.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

namespace chatterino {
namespace detail {
    // Frames
    Frames::Frames()
    {
        DebugCount::increase("images");
    }

    Frames::Frames(const QVector<Frame<QPixmap>> &frames)
        : items_(frames)
    {
        assertInGuiThread();
        DebugCount::increase("images");

        if (this->animated())
        {
            DebugCount::increase("animated images");

            this->gifTimerConnection_ =
                getApp()->emotes->gifTimer.signal.connect([this] {
                    this->advance();
                });
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
            this->durationOffset_ = std::min<int>(
                int(getApp()->emotes->gifTimer.position() % totalLength),
                60000);
        }
        this->processOffset();
    }

    Frames::~Frames()
    {
        assertInGuiThread();
        DebugCount::decrease("images");

        if (this->animated())
        {
            DebugCount::decrease("animated images");
        }

        this->gifTimerConnection_.disconnect();
    }

    void Frames::advance()
    {
        this->durationOffset_ += gifFrameLength;
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

        if (reader.imageCount() == 0)
        {
            qCDebug(chatterinoImage)
                << "Error while reading image" << url.string << ": '"
                << reader.errorString() << "'";
            return frames;
        }

        QImage image;
        for (int index = 0; index < reader.imageCount(); ++index)
        {
            if (reader.read(&image))
            {
                QPixmap::fromImage(image);

                int duration = std::max(20, reader.nextImageDelay());
                frames.push_back(Frame<QImage>{image, duration});
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
            queued.front().first(queued.front().second);
            queued.pop();

            if (++i > 50)
            {
                QTimer::singleShot(3, [&] {
                    assignDelayed(queued, mutex, loadedEventQueued);
                });
                return;
            }
        }

        getApp()->windows->forceLayoutChannelViews();
        loadedEventQueued = false;
    }

    template <typename Assign>
    auto makeConvertCallback(const QVector<Frame<QImage>> &parsed,
                             Assign assign)
    {
        return [parsed, assign] {
            // convert to pixmap
            auto frames = QVector<Frame<QPixmap>>();
            std::transform(parsed.begin(), parsed.end(),
                           std::back_inserter(frames), [](auto &frame) {
                               return Frame<QPixmap>{
                                   QPixmap::fromImage(frame.image),
                                   frame.duration};
                           });

            // put into stack
            static std::queue<std::pair<Assign, QVector<Frame<QPixmap>>>>
                queued;
            static std::mutex mutex;

            std::lock_guard<std::mutex> lock(mutex);
            queued.emplace(assign, frames);

            static std::atomic_bool loadedEventQueued{false};

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
    // run destructor of Frames in gui thread
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

ImagePtr Image::fromPixmap(const QPixmap &pixmap, qreal scale)
{
    auto result = ImagePtr(new Image(scale));

    result->setPixmap(pixmap);

    return result;
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

    this->load();

    return this->frames_->current();
}

void Image::load() const
{
    assertInGuiThread();

    if (this->shouldLoad_)
    {
        const_cast<Image *>(this)->shouldLoad_ = false;
        const_cast<Image *>(this)->actuallyLoad();
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
            auto parsed = detail::readFrames(reader, shared->url());

            postToThread(makeConvertCallback(parsed, [weak](auto frames) {
                if (auto shared = weak.lock())
                    shared->frames_ = std::make_unique<detail::Frames>(frames);
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

bool Image::operator==(const Image &other) const
{
    if (this->isEmpty() && other.isEmpty())
        return true;
    if (!this->url_.string.isEmpty() && this->url_ == other.url_)
        return true;
    if (this->frames_->first() == other.frames_->first())
        return true;

    return false;
}

bool Image::operator!=(const Image &other) const
{
    return !this->operator==(other);
}

}  // namespace chatterino
