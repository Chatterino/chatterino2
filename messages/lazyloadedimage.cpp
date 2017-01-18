#include "messages/lazyloadedimage.h"

#include "asyncexec.h"
#include "emotes.h"
#include "ircmanager.h"
#include "windows.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : pixmap(NULL)
    , url(url)
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(QPixmap *image, qreal scale,
                                 const QString &name, const QString &tooltip,
                                 const QMargins &margin, bool isHat)
    : pixmap(image)
    , url()
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(true)
{
}

void
LazyLoadedImage::loadImage()
{
    //    QThreadPool::globalInstance()->start(new LambdaQRunnable([=] {
    QUrl url(this->url);
    QNetworkRequest request(url);

    QNetworkReply *reply = IrcManager::getAccessManager().get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        QPixmap *pixmap = new QPixmap();
        pixmap->loadFromData(reply->readAll());

        if (pixmap->isNull()) {
            return;
        }

        this->pixmap = pixmap;
        Emotes::incGeneration();
        Windows::layoutVisibleChatWidgets();
    });
    //}));
}
}
}
