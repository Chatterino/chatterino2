#pragma once

#include <utility>
#include "Paint.hpp"

namespace chatterino {

class LinearGradientPaint : public Paint
{
public:
    LinearGradientPaint(const QString name, const std::optional<QColor> color,
                        const QGradientStops stops, const bool repeat,
                        const float angle, const std::vector<PaintDropShadow>);

    QBrush asBrush(const QColor userColor,
                   const QRectF drawingRect) const override;
    std::vector<PaintDropShadow> getDropShadows() const override;
    bool animated() const override;

private:
    const QString name_;
    const std::optional<QColor> color_;
    const QGradientStops stops_;
    const bool repeat_;
    const float angle_;

    const std::vector<PaintDropShadow> dropShadows_;
};

}  // namespace chatterino
