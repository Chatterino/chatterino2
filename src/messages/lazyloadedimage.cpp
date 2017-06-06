#include "messages/lazyloadedimage.h"

#include "asyncexec.h"
#include "emotemanager.h"
#include "ircmanager.h"
#include "util/urlfetch.h"
#include "windowmanager.h"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : _currentPixmap(NULL)
    , _currentFrame(0)
    , _currentFrameOffset(0)
    , _url(url)
    , _name(name)
    , _tooltip(tooltip)
    , _animated(false)
    , _margin(margin)
    , _ishat(isHat)
    , _scale(scale)
    , _isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(QPixmap *image, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : _currentPixmap(image)
    , _currentFrame(0)
    , _currentFrameOffset(0)
    , _name(name)
    , _tooltip(tooltip)
    , _animated(false)
    , _margin(margin)
    , _ishat(isHat)
    , _scale(scale)
    , _isLoading(true)
{
}

void LazyLoadedImage::loadImage()
{
    util::urlFetch(_url, [=](QNetworkReply &reply) {
        QByteArray array = reply.readAll();
        QBuffer buffer(&array);
        buffer.open(QIODevice::ReadOnly);

        QImage image;
        QImageReader reader(&buffer);

        bool first = true;

        for (int index = 0; index < reader.imageCount(); ++index) {
            if (reader.read(&image)) {
                auto pixmap = new QPixmap(QPixmap::fromImage(image));

                if (first) {
                    first = false;
                    _currentPixmap = pixmap;
                }

                FrameData data;
                data.duration = std::max(20, reader.nextImageDelay());
                data.image = pixmap;

                _allFrames.push_back(data);
            }
        }

        if (_allFrames.size() > 1) {
            _animated = true;

            EmoteManager::getInstance().getGifUpdateSignal().connect([this] { gifUpdateTimout(); });
        }

        EmoteManager::getInstance().incGeneration();
        WindowManager::getInstance().layoutVisibleChatWidgets();
    });
}

void LazyLoadedImage::gifUpdateTimout()
{
    _currentFrameOffset += GIF_FRAME_LENGTH;

    while (true) {
        if (_currentFrameOffset > _allFrames.at(_currentFrame).duration) {
            _currentFrameOffset -= _allFrames.at(_currentFrame).duration;
            _currentFrame = (_currentFrame + 1) % _allFrames.size();
        } else {
            break;
        }
    }

    _currentPixmap = _allFrames[_currentFrame].image;
}
}  // namespace messages
}  // namespace chatterino
