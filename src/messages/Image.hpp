#pragma once

#include "common/Aliases.hpp"
#include "common/Common.hpp"

#include <boost/variant.hpp>
#include <pajlada/signals/signal.hpp>
#include <QPixmap>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVector>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

namespace chatterino {
namespace detail {
    template <typename Image>
    struct Frame {
        Image image;
        int duration;
    };
    class Frames
    {
    public:
        Frames();
        Frames(QVector<Frame<QPixmap>> &&frames);
        ~Frames();

        Frames(const Frames &) = delete;
        Frames &operator=(const Frames &) = delete;

        Frames(Frames &&) = delete;
        Frames &operator=(Frames &&) = delete;

        void clear();
        bool empty() const;
        bool animated() const;
        void advance();
        std::optional<QPixmap> current() const;
        std::optional<QPixmap> first() const;

    private:
        int64_t memoryUsage() const;
        void processOffset();
        QVector<Frame<QPixmap>> items_;
        int index_{0};
        int durationOffset_{0};
        pajlada::Signals::Connection gifTimerConnection_;
    };
}  // namespace detail

class Image;
using ImagePtr = std::shared_ptr<Image>;

/// This class is thread safe.
class Image : public std::enable_shared_from_this<Image>
{
public:
    // Maximum amount of RAM used by the image in bytes.
    static constexpr int maxBytesRam = 20 * 1024 * 1024;

    ~Image();

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;

    Image(Image &&) = delete;
    Image &operator=(Image &&) = delete;

    static ImagePtr fromUrl(const Url &url, qreal scale = 1);
    static ImagePtr fromResourcePixmap(const QPixmap &pixmap, qreal scale = 1);
    static ImagePtr getEmpty();

    const Url &url() const;
    bool loaded() const;
    // either returns the current pixmap, or triggers loading it (lazy loading)
    std::optional<QPixmap> pixmapOrLoad() const;
    void load() const;
    qreal scale() const;
    bool isEmpty() const;
    int width() const;
    int height() const;
    bool animated() const;

    bool operator==(const Image &image) = delete;
    bool operator!=(const Image &image) = delete;

private:
    Image();
    Image(const Url &url, qreal scale);
    Image(qreal scale);

    void setPixmap(const QPixmap &pixmap);
    void actuallyLoad();
    void expireFrames();

    const Url url_{};
    const qreal scale_{1};
    std::atomic_bool empty_{false};

    mutable std::chrono::time_point<std::chrono::steady_clock> lastUsed_;

    bool shouldLoad_{false};

    // gui thread only
    std::unique_ptr<detail::Frames> frames_{};

    friend class ImageExpirationPool;
};

// forward-declarable function that calls Image::getEmpty() under the hood.
ImagePtr getEmptyImagePtr();

#ifndef DISABLE_IMAGE_EXPIRATION_POOL

class ImageExpirationPool
{
public:
    ImageExpirationPool();
    static ImageExpirationPool &instance();

    void addImagePtr(ImagePtr imgPtr);
    void removeImagePtr(Image *rawPtr);

    /**
     * @brief Frees frame data for all images that ImagePool deems to have expired.
     * 
     * Expiration is based on last accessed time of the Image, stored in Image::lastUsed_.
     * Must be ran in the GUI thread.
     */
    void freeOld();

    /*
     * Debug function that unloads all images in the pool. This is intended to
     * test for possible memory leaks from tracked images.
     */
    void freeAll();

    // Timer to periodically run freeOld()
    QTimer *freeTimer_;
    std::map<Image *, std::weak_ptr<Image>> allImages_;
    std::mutex mutex_;
};

#endif

}  // namespace chatterino
