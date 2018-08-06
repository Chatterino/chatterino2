#pragma once

#include "common/Common.hpp"

#include <QPixmap>
#include <QString>
#include <atomic>
#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>

#include "common/NullablePtr.hpp"

namespace chatterino {
namespace {
class Frame
{
public:
    explicit Frame(const QPixmap *nonOwning, int duration = 1);
    explicit Frame(std::unique_ptr<QPixmap> owning, int duration = 1);

    const QPixmap *pixmap() const;
    int duration() const;

private:
    const QPixmap *nonOwning_{nullptr};
    std::unique_ptr<QPixmap> owning_{};
    int duration_{};
};
class Frames
{
public:
    Frames();
    Frames(std::vector<Frame> &&frames);
    ~Frames();
    Frames(Frames &&other) = default;
    Frames &operator=(Frames &&other) = default;

    bool animated() const;
    void advance();
    const QPixmap *current() const;
    const QPixmap *first() const;

private:
    std::vector<Frame> items_;
    int index_{0};
    int timeOffset_{0};
};
}  // namespace

class Image;
using ImagePtr = std::shared_ptr<Image>;

class Image : public std::enable_shared_from_this<Image>, boost::noncopyable
{
public:
    static ImagePtr fromUrl(const Url &url, qreal scale = 1);
    static ImagePtr fromOwningPixmap(std::unique_ptr<QPixmap> pixmap, qreal scale = 1);
    static ImagePtr fromNonOwningPixmap(QPixmap *pixmap, qreal scale = 1);
    static ImagePtr getEmpty();

    const Url &url() const;
    const QPixmap *pixmap() const;
    qreal scale() const;
    bool empty() const;
    int width() const;
    int height() const;
    bool animated() const;

    bool operator==(const Image &image) const;
    bool operator!=(const Image &image) const;

private:
    Image();
    Image(const Url &url, qreal scale);
    Image(std::unique_ptr<QPixmap> owning, qreal scale);
    Image(QPixmap *nonOwning, qreal scale);

    void load();

    Url url_{};
    qreal scale_{1};
    bool empty_{false};
    bool shouldLoad_{false};
    Frames frames_{};
    QObject object_{};

    static std::atomic<bool> loadedEventQueued;
};
}  // namespace chatterino
