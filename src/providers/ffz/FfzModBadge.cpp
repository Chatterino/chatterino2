#include "FfzModBadge.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QJsonObject>
#include <QPainter>
#include <QString>

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

FfzModBadge::FfzModBadge(const QString &channelName)
    : channelName_(channelName)
{
}

void FfzModBadge::loadCustomModBadge()
{
    static QString partialUrl("https://cdn.frankerfacez.com/room-badge/mod/");

    QString url = partialUrl + channelName_ + "/1";
    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.onSuccess([this, url](auto result) -> Outcome {
        auto data = result.getData();

        QBuffer buffer(const_cast<QByteArray *>(&data));
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        if (reader.imageCount() == 0)
            return Failure;

        QPixmap badgeOverlay = QPixmap::fromImageReader(&reader);
        QPixmap badgePixmap(18, 18);

        // the default mod badge green color
        badgePixmap.fill(QColor("#34AE0A"));
        QPainter painter(&badgePixmap);
        QRectF rect(0, 0, 18, 18);
        painter.drawPixmap(rect, badgeOverlay, rect);

        auto emote = Emote{{""},
                           ImageSet{Image::fromPixmap(badgePixmap)},
                           Tooltip{"Twitch Channel Moderator"},
                           Url{url}};

        this->badge_ = std::make_shared<Emote>(emote);

        return Success;
    });

    req.execute();
}

EmotePtr FfzModBadge::badge() const
{
    return this->badge_;
}

}  // namespace chatterino
