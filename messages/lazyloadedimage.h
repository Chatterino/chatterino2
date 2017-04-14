#ifndef LAZYLOADEDIMAGE_H
#define LAZYLOADEDIMAGE_H

#include <QPixmap>
#include <QString>

namespace chatterino {
namespace messages {

class LazyLoadedImage : QObject
{
public:
    explicit LazyLoadedImage(const QString &_url, qreal _scale = 1, const QString &_name = "",
                             const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                             bool isHat = false);
    explicit LazyLoadedImage(QPixmap *_currentPixmap, qreal _scale = 1, const QString &_name = "",
                             const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                             bool isHat = false);

    const QPixmap *getPixmap()
    {
        if (!_isLoading) {
            _isLoading = true;

            loadImage();
        }
        return _currentPixmap;
    }

    qreal getScale() const
    {
        return _scale;
    }

    const QString &getUrl() const
    {
        return _url;
    }

    const QString &getName() const
    {
        return _name;
    }

    const QString &getTooltip() const
    {
        return _tooltip;
    }

    const QMargins &getMargin() const
    {
        return _margin;
    }

    bool getAnimated() const
    {
        return _animated;
    }

    bool isHat() const
    {
        return _ishat;
    }

    int getWidth() const
    {
        if (_currentPixmap == NULL) {
            return 16;
        }
        return _currentPixmap->width();
    }

    int getHeight() const
    {
        if (_currentPixmap == NULL) {
            return 16;
        }
        return _currentPixmap->height();
    }

private:
    struct FrameData {
        QPixmap *image;
        int duration;
    };

    QPixmap *_currentPixmap;
    std::vector<FrameData> _allFrames;
    int _currentFrame;
    int _currentFrameOffset;

    QString _url;
    QString _name;
    QString _tooltip;
    bool _animated;
    QMargins _margin;
    bool _ishat;
    qreal _scale;

    bool _isLoading;

    void loadImage();

    void gifUpdateTimout();
};
}  // namespace  messages
}  // namespace  chatterino

#endif  // LAZYLOADEDIMAGE_H
