#include "LinearGradientPaint.hpp"

namespace chatterino {

LinearGradientPaint::LinearGradientPaint(
    const QString name, const std::optional<QColor> color,
    const QGradientStops stops, const bool repeat, const float angle,
    const std::vector<PaintDropShadow> dropShadows)
    : Paint()
    , name_(name)
    , color_(color)
    , stops_(stops)
    , repeat_(repeat)
    , angle_(angle)
    , dropShadows_(dropShadows)
{
}

bool LinearGradientPaint::animated() const
{
    return false;
}

QBrush LinearGradientPaint::asBrush(const QColor userColor,
                                    const QRectF drawingRect) const
{
    QPointF startPoint = drawingRect.bottomLeft();
    QPointF endPoint = drawingRect.topRight();

    // NOTE: use modulo to also account for angles >= 360 degrees
    const int angleStep = int(this->angle_ / 90) % 4;
    if (angleStep == 1)  // 90-179 degrees
    {
        startPoint = drawingRect.topLeft();
        endPoint = drawingRect.bottomRight();
    }
    if (angleStep == 2)  // 180-269 degrees
    {
        startPoint = drawingRect.topRight();
        endPoint = drawingRect.bottomLeft();
    }
    if (angleStep == 3)  // 270-359 degrees
    {
        startPoint = drawingRect.bottomRight();
        endPoint = drawingRect.topLeft();
    }

    QLineF gradientAxis;
    gradientAxis.setP1(drawingRect.center());
    gradientAxis.setAngle(90.0f - this->angle_);

    QLineF colorStartAxis;
    colorStartAxis.setP1(startPoint);
    colorStartAxis.setAngle(-this->angle_);

    QLineF colorStopAxis;
    colorStopAxis.setP1(endPoint);
    colorStopAxis.setAngle(-this->angle_);

    QPointF gradientStart;
    QPointF gradientEnd;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    gradientAxis.intersects(colorStartAxis, &gradientStart);
    gradientAxis.intersects(colorStopAxis, &gradientEnd);
#else
    gradientAxis.intersect(colorStartAxis, &gradientStart);
    gradientAxis.intersect(colorStopAxis, &gradientEnd);
#endif

    if (this->repeat_)
    {
        QLineF gradientLine(gradientStart, gradientEnd);
        gradientStart = gradientLine.pointAt(this->stops_.front().first);
        gradientEnd = gradientLine.pointAt(this->stops_.back().first);
    }

    QLinearGradient gradient(gradientStart, gradientEnd);

    const auto spread =
        this->repeat_ ? QGradient::RepeatSpread : QGradient::PadSpread;
    gradient.setSpread(spread);

    for (auto const &[position, color] : this->stops_)
    {
        const auto combinedColor = this->overlayColors(userColor, color);
        const float offsetPosition =
            this->repeat_
                ? this->offsetRepeatingStopPosition(position, this->stops_)
                : position;
        gradient.setColorAt(offsetPosition, combinedColor);
    }

    return QBrush(gradient);
}

std::vector<PaintDropShadow> LinearGradientPaint::getDropShadows() const
{
    return this->dropShadows_;
}

}  // namespace chatterino
