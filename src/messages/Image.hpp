#pragma once

#include <QPixmap>
#include <QString>
#include <QThread>
#include <QVector>
#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <memory>
#include <mutex>
#include <pajlada/signals/signal.hpp>

#include "common/Aliases.hpp"
#include "common/NullablePtr.hpp"

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
        Frames(const QVector<Frame<QPixmap>> &frames);
        ~Frames();

        bool animated() const;
        void advance();
        boost::optional<QPixmap> current() const;
        boost::optional<QPixmap> first() const;

    private:
        QVector<Frame<QPixmap>> items_;
        int index_{0};
        int durationOffset_{0};
        pajlada::Signals::Connection gifTimerConnection_;
    };
}  // namespace detail

class Image;
using ImagePtr = std::shared_ptr<Image>;

class Image : public std::enable_shared_from_this<Image>, boost::noncopyable
{
public:
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

    Url url_{};
    qreal scale_{1};
    bool empty_{false};
    bool shouldLoad_{false};
    std::unique_ptr<detail::Frames> frames_{};
    QObject object_{};
};
}  // namespace chatterino
