#include "lazyloadedimage.h"

#include "asyncexec.h"
#include "emotes.h"
#include "ircmanager.h"
#include "windows.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <functional>

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : m_pixmap(NULL)
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

LazyLoadedImage::LazyLoadedImage(QPixmap *image, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : m_pixmap(image)
    , m_url()
    , m_name(name)
    , m_tooltip(tooltip)
    , m_animated(false)
    , m_margin(margin)
    , m_ishat(isHat)
    , m_scale(scale)
    , m_isLoading(true)
{
}

void
LazyLoadedImage::loadImage()
{
    //    QThreadPool::globalInstance()->start(new LambdaQRunnable([=] {
    QUrl url(m_url);
    QNetworkRequest request(url);

    QNetworkReply *reply = IrcManager::accessManager().get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        QPixmap *pixmap = new QPixmap();
        pixmap->loadFromData(reply->readAll());

        if (pixmap->isNull()) {
            return;
        }

        m_pixmap = pixmap;
        Emotes::incGeneration();
        Windows::layoutVisibleChatWidgets();
    });
    //}));
}
