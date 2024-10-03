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
    virtual const std::vector<PaintDropShadow> &getDropShadows() const = 0;
    virtual bool animated() const = 0;

    QPixmap getPixmap(const QString &text, const QFont &font, QColor userColor,
                      QSize size, float scale, float dpr) const;

    Paint(QString id)
        : id(std::move(id)){};
    virtual ~Paint() = default;

    Paint(const Paint &) = default;
    Paint(Paint &&) = delete;
    Paint &operator=(const Paint &) = default;
    Paint &operator=(Paint &&) = delete;

    QString id;

protected:
    static QColor overlayColors(QColor background, QColor foreground);
    static qreal offsetRepeatingStopPosition(qreal position,
                                             const QGradientStops &stops);
};

}  // namespace chatterino
