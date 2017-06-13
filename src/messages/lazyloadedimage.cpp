#include "messages/lazyloadedimage.hpp"
#include "asyncexec.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                                 const QString &url, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : emoteManager(_emoteManager)
    , windowManager(_windowManager)
    , _currentPixmap(nullptr)
    , _url(url)
    , _name(name)
    , _tooltip(tooltip)
    , _margin(margin)
    , _ishat(isHat)
    , _scale(scale)
    , _isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(EmoteManager &_emoteManager, WindowManager &_windowManager,
                                 QPixmap *image, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : emoteManager(_emoteManager)
    , windowManager(_windowManager)
    , _currentPixmap(image)
    , _name(name)
    , _tooltip(tooltip)
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

            this->emoteManager.getGifUpdateSignal().connect([this] {
                gifUpdateTimout();  //
            });
        }

        this->emoteManager.incGeneration();
        this->windowManager.layoutVisibleChatWidgets();
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
