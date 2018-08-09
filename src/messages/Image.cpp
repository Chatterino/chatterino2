#include "messages/Image.hpp"

#include "Application.hpp"
#include "common/NetworkRequest.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Benchmark.hpp"
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
std::vector<ParseFrame> readFrames(QImageReader &reader, const Url &url)
{
    std::vector<ParseFrame> frames;

    if (reader.imageCount() == 0) {
        Log("Error while reading image {}: '{}'", url.string,
            reader.errorString());
        return frames;
    }

    QImage image;
    for (int index = 0; index < reader.imageCount(); ++index) {
        if (reader.read(&image)) {
            QPixmap::fromImage(image);

            int duration = std::max(20, reader.nextImageDelay());
            frames.push_back(ParseFrame{image, duration});
        }
    }

    if (frames.size() == 0) {
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

// parsed
template <typename Assign>
void asd(std::queue<std::pair<Assign, std::vector<Frame>>> &queued,
         std::mutex &mutex, std::atomic_bool &loadedEventQueued)
{
    std::lock_guard<std::mutex> lock(mutex);
    int i = 0;

    while (!queued.empty()) {
        queued.front().first(std::move(queued.front().second));
        queued.pop();

        if (++i > 50) {
            QTimer::singleShot(3,
                               [&] { asd(queued, mutex, loadedEventQueued); });
            return;
        }
    }

    getApp()->windows->forceLayoutChannelViews();
    loadedEventQueued = false;
}

template <typename Assign>
auto makeConvertCallback(std::vector<ParseFrame> parsed, Assign assign)
{
    return [parsed = std::move(parsed), assign] {
        // BenchmarkGuard guard("convert image");

        // convert to pixmap
        auto frames = std::vector<Frame>();
        std::transform(parsed.begin(), parsed.end(), std::back_inserter(frames),
                       [](auto &frame) {
                           return Frame{std::make_unique<QPixmap>(
                                            QPixmap::fromImage(frame.image)),
                                        frame.duration};
                       });

        // put into stack
        static std::queue<std::pair<Assign, std::vector<Frame>>> queued;
        static std::mutex mutex;

        std::lock_guard<std::mutex> lock(mutex);
        queued.emplace(assign, std::move(frames));

        static std::atomic_bool loadedEventQueued{false};

        if (!loadedEventQueued) {
            loadedEventQueued = true;

            QTimer::singleShot(100,
                               [=] { asd(queued, mutex, loadedEventQueued); });
        }
    };
}
}  // namespace

// IMAGE2
ImagePtr Image::fromUrl(const Url &url, qreal scale)
{
    static std::unordered_map<Url, std::weak_ptr<Image>> cache;
    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);

    auto shared = cache[url].lock();

    if (!shared) {
        cache[url] = shared = ImagePtr(new Image(url, scale));
    } else {
        // Warn("same image loaded multiple times: {}", url.string);
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
    req.onSuccess([that = this, weak = weakOf(this)](auto result) -> Outcome {
        assertInGuiThread();

        auto shared = weak.lock();
        if (!shared) return Failure;

        static auto parseThread = [] {
            auto thread = std::make_unique<QThread>();
            thread->start();
            return thread;
        }();

        postToThread(
            [data = result.getData(), weak, that] {
                // BenchmarkGuard guard("parse image");

                // const cast since we are only reading from it
                QBuffer buffer(const_cast<QByteArray *>(&data));
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);
                auto parsed = readFrames(reader, that->url());

                postToThread(
                    makeConvertCallback(std::move(parsed), [weak](auto frames) {
                        if (auto shared = weak.lock())
                            shared->frames_ = std::move(frames);
                    }));
            },
            parseThread.get());

        return Success;
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
