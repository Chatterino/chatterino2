#ifndef LAZYLOADEDIMAGE_H
#define LAZYLOADEDIMAGE_H

#include <QPixmap>
#include <QString>

namespace chatterino {
namespace messages {

class LazyLoadedImage
{
public:
    explicit LazyLoadedImage(const QString &url, qreal scale = 1,
                             const QString &name = "",
                             const QString &tooltip = "",
                             const QMargins &margin = QMargins(),
                             bool isHat = false);
    explicit LazyLoadedImage(QPixmap *pixmap, qreal scale = 1,
                             const QString &name = "",
                             const QString &tooltip = "",
                             const QMargins &margin = QMargins(),
                             bool isHat = false);

    const QPixmap *
    getPixmap()
    {
        if (!isLoading) {
            isLoading = true;

            loadImage();
        }
        return pixmap;
    }

    qreal
    getScale() const
    {
        return scale;
    }

    const QString &
    getUrl() const
    {
        return url;
    }

    const QString &
    getName() const
    {
        return name;
    }

    const QString &
    getTooltip() const
    {
        return tooltip;
    }

    const QMargins &
    getMargin() const
    {
        return margin;
    }

    bool
    getAnimated() const
    {
        return animated;
    }

    bool
    getIsHat() const
    {
        return ishat;
    }

    int
    getWidth() const
    {
        if (pixmap == NULL) {
            return 16;
        }
        return pixmap->width();
    }

    int
    getHeight() const
    {
        if (pixmap == NULL) {
            return 16;
        }
        return pixmap->height();
    }

private:
    QPixmap *pixmap;

    QString url;
    QString name;
    QString tooltip;
    bool animated;
    QMargins margin;
    bool ishat;
    qreal scale;

    bool isLoading;

    void loadImage();
};
}
}

#endif  // LAZYLOADEDIMAGE_H
