#include "providers/seventv/paints/Paint.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "singletons/Theme.hpp"

#include <QLabel>
#include <QPainter>

namespace chatterino {

using namespace literals;

QPixmap Paint::getPixmap(const QString &text, const QFont &font,
                         QColor userColor, QSize size, float scale) const
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setRenderHint(QPainter::SmoothPixmapTransform);
    pixmapPainter.setFont(font);

    // NOTE: draw colon separately from the nametag
    // otherwise the paint would extend onto the colon
    bool drawColon = false;
    QRectF nametagBoundingRect = pixmap.rect();
    QString nametagText = text;
    if (nametagText.endsWith(':'))
    {
        drawColon = true;
        nametagText = nametagText.chopped(1);
        nametagBoundingRect = pixmapPainter.boundingRect(
            QRectF(0, 0, 10000, 10000), nametagText,
            QTextOption(Qt::AlignLeft | Qt::AlignTop));
    }

    QPen pen;
    const QBrush brush = this->asBrush(userColor, nametagBoundingRect);
    pen.setBrush(brush);
    pixmapPainter.setPen(pen);

    pixmapPainter.drawText(nametagBoundingRect, nametagText,
                           QTextOption(Qt::AlignLeft | Qt::AlignTop));
    pixmapPainter.end();

    for (const auto &shadow : this->getDropShadows())
    {
        if (!shadow.isValid())
        {
            continue;
        }

        // HACK: create a QLabel from the pixmap to apply drop shadows
        QLabel label;

        auto scaledShadow = shadow.scaled(
            scale / static_cast<float>(label.devicePixelRatioF()));

        // NOTE: avoid scaling issues on high DPI displays
        pixmap.setDevicePixelRatio(label.devicePixelRatioF());

        label.setPixmap(pixmap);

        QGraphicsDropShadowEffect dropShadow;
        scaledShadow.apply(dropShadow);
        label.setGraphicsEffect(&dropShadow);

        pixmap = label.grab();
        pixmap.setDevicePixelRatio(1);
    }

    if (drawColon)
    {
        auto colonColor = getApp()->getThemes()->messages.textColors.regular;

        pixmapPainter.begin(&pixmap);

        pixmapPainter.setPen(QPen(colonColor));
        pixmapPainter.setFont(font);

        QRectF colonBoundingRect(nametagBoundingRect.right(), 0, 10000, 10000);
        pixmapPainter.drawText(colonBoundingRect, u":"_s,
                               QTextOption(Qt::AlignLeft | Qt::AlignTop));
        pixmapPainter.end();
    }

    return pixmap;
}

QColor Paint::overlayColors(QColor background, QColor foreground)
{
    auto alpha = foreground.alphaF();

    auto r = (1 - alpha) * static_cast<float>(background.red()) +
             alpha * static_cast<float>(foreground.red());
    auto g = (1 - alpha) * static_cast<float>(background.green()) +
             alpha * static_cast<float>(foreground.green());
    auto b = (1 - alpha) * static_cast<float>(background.blue()) +
             alpha * static_cast<float>(foreground.blue());

    return {static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)};
}

qreal Paint::offsetRepeatingStopPosition(const qreal position,
                                         const QGradientStops &stops)
{
    const qreal gradientStart = stops.first().first;
    const qreal gradientEnd = stops.last().first;
    const qreal gradientLength = gradientEnd - gradientStart;
    const qreal offsetPosition = (position - gradientStart) / gradientLength;

    return offsetPosition;
}

}  // namespace chatterino
