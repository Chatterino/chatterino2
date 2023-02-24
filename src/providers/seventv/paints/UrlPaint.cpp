#include "providers/seventv/paints/UrlPaint.hpp"

#include <QPainter>

#include <utility>

namespace chatterino {

UrlPaint::UrlPaint(QString name, QString id, ImagePtr image,
                   std::vector<PaintDropShadow> dropShadows)
    : Paint(std::move(id))
    , name_(std::move(name))
    , image_(std::move(image))
    , dropShadows_(std::move(dropShadows))
{
}

bool UrlPaint::animated() const
{
    return image_->animated();
}

QBrush UrlPaint::asBrush(const QColor userColor, const QRectF drawingRect) const
{
    if (auto paintPixmap = this->image_->pixmapOrLoad())
    {
        paintPixmap = paintPixmap->scaledToWidth(drawingRect.width());

        QPixmap userColorPixmap = QPixmap(paintPixmap->size());
        userColorPixmap.fill(userColor);

        QPainter painter(&userColorPixmap);
        painter.drawPixmap(0, 0, *paintPixmap);

        const QPixmap combinedPixmap = userColorPixmap.copy(
            QRect(0, 0, drawingRect.width(), drawingRect.height()));
        return QBrush(combinedPixmap);
    }

    return QBrush(userColor);
}

std::vector<PaintDropShadow> UrlPaint::getDropShadows() const
{
    return this->dropShadows_;
}

}  // namespace chatterino
