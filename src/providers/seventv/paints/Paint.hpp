#pragma once

#include "providers/seventv/paints/PaintDropShadow.hpp"

#include <QBrush>
#include <QFont>

#include <vector>

namespace chatterino {

class Paint
{
public:
    virtual QBrush asBrush(QColor userColor, QRectF drawingRect) const = 0;
    virtual std::vector<PaintDropShadow> getDropShadows() const = 0;
    virtual bool animated() const = 0;

    QPixmap getPixmap(QString text, QFont font, QColor userColor, QSize size,
                      float scale) const;

    Paint(QString id)
        : id(std::move(id)){};
    virtual ~Paint() = default;

    QString id;

protected:
    QColor overlayColors(const QColor background,
                         const QColor foreground) const;
    float offsetRepeatingStopPosition(const float position,
                                      const QGradientStops stops) const;
};

}  // namespace chatterino
