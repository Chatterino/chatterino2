#include "lazyloadedimage.h"

LazyLoadedImage::LazyLoadedImage(const QString& url, qreal scale, const QString& name, const QString& tooltip, const QMargins& margin, bool isHat)
    : m_image(NULL)
    , m_url(url)
    , m_name(name)
    , m_tooltip(tooltip)
    , m_animated(false)
    , m_margin(margin)
    , m_ishat(isHat)
    , m_scale(scale)
{

}

LazyLoadedImage::LazyLoadedImage(QImage *image, qreal scale, const QString& name, const QString& tooltip, const QMargins& margin, bool isHat)
    : m_name(name)
    , m_tooltip(tooltip)
    , m_animated(false)
    , m_margin(margin)
    , m_ishat(isHat)
    , m_scale(scale)
    , m_image(image)
{

}
