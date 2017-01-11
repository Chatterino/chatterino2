#include "lazyloadedimage.h"

#include "asyncexec.h"
#include "ircmanager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <functional>

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : m_image(NULL)
    , m_url(url)
    , m_name(name)
    , m_tooltip(tooltip)
    , m_animated(false)
    , m_margin(margin)
    , m_ishat(isHat)
    , m_scale(scale)
    , m_isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(QImage *image, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : m_name(name)
    , m_tooltip(tooltip)
    , m_animated(false)
    , m_margin(margin)
    , m_ishat(isHat)
    , m_scale(scale)
    , m_image(image)
    , m_isLoading(true)
{
}

void
LazyLoadedImage::loadImage()
{
    QString url = m_url;

    async_exec([url] {
        QNetworkRequest req(QUrl(url));

        QNetworkReply *reply = IrcManager::accessManager().get(req);

        QObject::connect(reply, &QNetworkReply::finished, [=] {});
    })
}
