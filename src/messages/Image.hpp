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

class Image;
using ImagePtr = std::shared_ptr<Image>;

class Image : public std::enable_shared_from_this<Image>, boost::noncopyable
{
public:
    static ImagePtr fromUrl(const Url &url, qreal scale = 1);
    static ImagePtr fromOwningPixmap(std::unique_ptr<QPixmap> pixmap, qreal scale = 1);
    static ImagePtr fromNonOwningPixmap(QPixmap *pixmap, qreal scale = 1);
    static ImagePtr getEmpty();

    const Url &getUrl() const;
    NullablePtr<const QPixmap> getPixmap() const;
    qreal getScale() const;
    bool isAnimated() const;
    int getWidth() const;
    int getHeight() const;
    bool isLoaded() const;
    bool isError() const;
    bool isValid() const;
    bool isNull() const;

    bool operator==(const Image &image) const;
    bool operator!=(const Image &image) const;

private:
    class Frame
    {
    public:
        QPixmap *getPixmap() const;
        int getDuration() const;

        Frame(QPixmap *nonOwning, int duration = 1);
        Frame(std::unique_ptr<QPixmap> nonOwning, int duration = 1);

    private:
        QPixmap *nonOwning_;
        std::unique_ptr<QPixmap> owning_;
        int duration_;
    };

    Image();
    Image(const Url &url, qreal scale);
    Image(std::unique_ptr<QPixmap> owning, qreal scale);
    Image(QPixmap *nonOwning, qreal scale);

    void load();
    Outcome parse(const QByteArray &data);
    std::vector<Frame> readFrames(QImageReader &reader);
    Outcome setFrames(std::vector<Frame> frames);
    void updateAnimation();
    void queueLoadedEvent();

    Url url_;
    bool isLoaded_{false};
    bool isLoading_{false};
    bool isAnimated_{false};
    bool isError_{false};
    bool isNull_ = false;
    qreal scale_ = 1;
    QObject object_;

    std::vector<Frame> frames_;
    std::mutex framesMutex_;
    NullablePtr<QPixmap> currentFramePixmap_;
    int currentFrameIndex_ = 0;
    int currentFrameOffset_ = 0;

    static std::atomic<bool> loadedEventQueued;
};
}  // namespace chatterino
