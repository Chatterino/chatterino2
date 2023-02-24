#pragma once

#include "providers/seventv/paints/Paint.hpp"

namespace chatterino {

class RadialGradientPaint : public Paint
{
public:
    RadialGradientPaint(QString name, QString id, QGradientStops stops,
                        bool repeat, std::vector<PaintDropShadow>);

    QBrush asBrush(QColor userColor, QRectF drawingRect) const override;
    std::vector<PaintDropShadow> getDropShadows() const override;
    bool animated() const override;

private:
    const QString name_;
    const QGradientStops stops_;
    const bool repeat_;

    const std::vector<PaintDropShadow> dropShadows_;
};

}  // namespace chatterino
