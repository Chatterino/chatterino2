#ifndef LAZYLOADEDIMAGE_H
#define LAZYLOADEDIMAGE_H

#include <QPixmap>
#include <QString>

class LazyLoadedImage
{
public:
    LazyLoadedImage(const QString &url, qreal scale = 1,
                    const QString &name = "", const QString &tooltip = "",
                    const QMargins &margin = QMargins(), bool isHat = false);
    LazyLoadedImage(QPixmap *pixmap, qreal scale = 1, const QString &name = "",
                    const QString &tooltip = "",
                    const QMargins &margin = QMargins(), bool isHat = false);

    const QPixmap *
    pixmap()
    {
        if (!m_isLoading) {
            m_isLoading = true;

            loadImage();
        }
        return m_pixmap;
    }

    qreal
    scale() const
    {
        return m_scale;
    }

    const QString &
    url() const
    {
        return m_url;
    }

    const QString &
    name() const
    {
        return m_name;
    }

    const QString &
    tooltip() const
    {
        return m_tooltip;
    }

    const QMargins &
    margin() const
    {
        return m_margin;
    }

    bool
    animated() const
    {
        return m_animated;
    }

    bool
    isHat() const
    {
        return m_ishat;
    }

    int
    width() const
    {
        if (m_pixmap == NULL) {
            return 16;
        }
        return m_pixmap->width();
    }

    int
    height() const
    {
        if (m_pixmap == NULL) {
            return 16;
        }
        return m_pixmap->height();
    }

private:
    QPixmap *m_pixmap;

    QString m_url;
    QString m_name;
    QString m_tooltip;
    bool m_animated;
    QMargins m_margin;
    bool m_ishat;
    qreal m_scale;

    bool m_isLoading;

    void loadImage();
};

#endif  // LAZYLOADEDIMAGE_H
