#pragma once

#include "Paint.hpp"

namespace chatterino {

class RadialGradientPaint : public Paint
{
public:
    RadialGradientPaint(const QString name, const QGradientStops stops,
                        const bool repeat, const std::vector<PaintDropShadow>);

    QBrush asBrush(const QColor userColor,
                   const QRectF drawingRect) const override;
    std::vector<PaintDropShadow> getDropShadows() const override;
    bool animated() const override;

private:
    const QString name_;
    const QGradientStops stops_;
    const bool repeat_;

    const std::vector<PaintDropShadow> dropShadows_;
};

}  // namespace chatterino
