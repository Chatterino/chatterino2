#include "providers/seventv/paints/RadialGradientPaint.hpp"

namespace chatterino {

RadialGradientPaint::RadialGradientPaint(
    QString name, QString id, QGradientStops stops, bool repeat,
    std::vector<PaintDropShadow> dropShadows)
    : Paint(std::move(id))
    , name_(std::move(name))
    , stops_(std::move(stops))
    , repeat_(repeat)
    , dropShadows_(std::move(dropShadows))
{
}

QBrush RadialGradientPaint::asBrush(QColor userColor, QRectF drawingRect) const
{
    double x = drawingRect.x() + drawingRect.width() / 2;
    double y = drawingRect.y() + drawingRect.height() / 2;

    double radius = std::max(drawingRect.width(), drawingRect.height()) / 2;
    radius = this->repeat_ ? radius * this->stops_.back().first : radius;

    QRadialGradient gradient(x, y, radius);

    auto spread =
        this->repeat_ ? QGradient::RepeatSpread : QGradient::PadSpread;
    gradient.setSpread(spread);

    for (const auto &[position, color] : this->stops_)
    {
        auto combinedColor =
            RadialGradientPaint::overlayColors(userColor, color);
        auto offsetPosition =
            this->repeat_ ? RadialGradientPaint::offsetRepeatingStopPosition(
                                position, this->stops_)
                          : position;

        gradient.setColorAt(offsetPosition, combinedColor);
    }

    return {gradient};
}

bool RadialGradientPaint::animated() const
{
    return false;
}

const std::vector<PaintDropShadow> &RadialGradientPaint::getDropShadows() const
{
    return this->dropShadows_;
}

}  // namespace chatterino
