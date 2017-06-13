#pragma once

#include <QPixmap>
#include <QString>

namespace chatterino {

class EmoteManager;
class WindowManager;

namespace messages {

class LazyLoadedImage : QObject
{
public:
    LazyLoadedImage() = delete;

    explicit LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                             const QString &_url, qreal _scale = 1, const QString &_name = "",
                             const QString &_tooltip = "", const QMargins &_margin = QMargins(),
                             bool isHat = false);

    explicit LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                             QPixmap *_currentPixmap, qreal _scale = 1, const QString &_name = "",
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
        if (_currentPixmap == nullptr) {
            return 16;
        }
        return _currentPixmap->width();
    }

    int getHeight() const
    {
        if (_currentPixmap == nullptr) {
            return 16;
        }
        return _currentPixmap->height();
    }

private:
    EmoteManager &emoteManager;
    WindowManager &windowManager;

    struct FrameData {
        QPixmap *image;
        int duration;
    };

    QPixmap *_currentPixmap;
    std::vector<FrameData> _allFrames;
    int _currentFrame = 0;
    int _currentFrameOffset = 0;

    QString _url;
    QString _name;
    QString _tooltip;
    bool _animated = false;
    QMargins _margin;
    bool _ishat;
    qreal _scale;

    bool _isLoading;

    void loadImage();

    void gifUpdateTimout();
};

}  // namespace messages
}  // namespace chatterino
