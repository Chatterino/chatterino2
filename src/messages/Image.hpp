#pragma once

#include <QPixmap>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <pajlada/signals/signal.hpp>
#include <set>

#include "common/Aliases.hpp"
#include "common/Common.hpp"

namespace chatterino {
namespace detail {
    template <typename Image>
    struct Frame {
        Image image;
        int duration;
    };
    class Frames : boost::noncopyable
    {
    public:
        Frames();
        Frames(QVector<Frame<QPixmap>> &&frames);
        ~Frames();

        bool empty() const;
        bool animated() const;
        void advance();
        boost::optional<QPixmap> current() const;
        boost::optional<QPixmap> first() const;

    private:
        void processOffset();
        QVector<Frame<QPixmap>> items_;
        int index_{0};
        int durationOffset_{0};
        pajlada::Signals::Connection gifTimerConnection_;
    };
}  // namespace detail

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ImagePool : private QObject
{
private:
    friend class Image;

    ImagePool();
    static ImagePool &instance();

    /**
     * @brief Stores a reference to the given Image in the ImagePool.
     * 
     * The caller is responsible that the pointer will be valid for as long as
     * it exists within ImagePool. This generally means it must call removeImagePtr
     * before the Image's destructor is ran. 
     */
    void addImagePtr(Image *imgPtr);

    /**
     * @brief Removes the reference for the given Image, if it exists.
     */
    void removeImagePtr(Image *imgPtr);

    /**
     * @brief Frees frame data for all images that ImagePool deems to have expired.
     * 
     * Expiration is based on last accessed time of the Image, stored in Image::lastUsed_.
     */
    void freeOld();

private:
    // Timer to periodically run freeOld()
    QTimer *freeTimer_;
    // Set of all tracked Images. We use an ordered set for its lower memory usage
    // and O(log n) arbitrary removal time.
    std::set<Image *> allImages_;
};

/// This class is thread safe.
class Image : public std::enable_shared_from_this<Image>, boost::noncopyable
{
public:
    // Maximum amount of RAM used by the image in bytes.
    static constexpr int maxBytesRam = 20 * 1024 * 1024;

    ~Image();

    static ImagePtr fromUrl(const Url &url, qreal scale = 1);
    static ImagePtr fromPixmap(const QPixmap &pixmap, qreal scale = 1);
    static ImagePtr getEmpty();

    const Url &url() const;
    bool loaded() const;
    // either returns the current pixmap, or triggers loading it (lazy loading)
    boost::optional<QPixmap> pixmapOrLoad() const;
    void load() const;
    qreal scale() const;
    bool isEmpty() const;
    int width() const;
    int height() const;
    bool animated() const;

    bool operator==(const Image &image) const;
    bool operator!=(const Image &image) const;

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

    bool canExpire_;
    bool shouldLoad_{false};

    // gui thread only
    std::unique_ptr<detail::Frames> frames_{};

    friend class ImagePool;
};
}  // namespace chatterino
