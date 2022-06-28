#pragma once

#include <QBrush>
#include <QFont>
#include <vector>
#include "providers/seventv/paints/PaintDropShadow.hpp"

namespace chatterino {

class Paint
{
public:
    virtual QBrush asBrush(const QColor userColor,
                           const QRectF drawingRect) const = 0;
    virtual std::vector<PaintDropShadow> getDropShadows() const = 0;
    virtual bool animated() const = 0;

    QPixmap getPixmap(const QString text, const QFont font,
                      const QColor userColor, const QSize size,
                      const float scale) const;

    virtual ~Paint(){};

protected:
    QColor overlayColors(const QColor background,
                         const QColor foreground) const;
    float offsetRepeatingStopPosition(const float position,
                                      const QGradientStops stops) const;
};

}  // namespace chatterino
