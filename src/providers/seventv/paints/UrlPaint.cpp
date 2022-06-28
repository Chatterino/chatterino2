#include "UrlPaint.hpp"

#include <QPainter>

namespace chatterino {

UrlPaint::UrlPaint(const QString name, const ImagePtr image,
                   const std::vector<PaintDropShadow> dropShadows)
    : Paint()
    , name_(name)
    , image_(image)
    , dropShadows_(dropShadows)
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
