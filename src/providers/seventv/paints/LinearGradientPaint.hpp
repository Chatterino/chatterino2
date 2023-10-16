#pragma once

#include "Paint.hpp"

#include <optional>

namespace chatterino {

class LinearGradientPaint : public Paint
{
public:
    LinearGradientPaint(QString name, QString id, std::optional<QColor> color,
                        QGradientStops stops, bool repeat, float angle,
                        std::vector<PaintDropShadow> dropShadows);

    QBrush asBrush(QColor userColor, QRectF drawingRect) const override;
    const std::vector<PaintDropShadow> &getDropShadows() const override;
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
