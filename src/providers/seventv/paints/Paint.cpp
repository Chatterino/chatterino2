#include "providers/seventv/paints/Paint.hpp"

#include "Application.hpp"
#include "singletons/Theme.hpp"

#include <QLabel>
#include <QPainter>

namespace chatterino {

QPixmap Paint::getPixmap(const QString text, const QFont font,
                         const QColor userColor, const QSize size,
                         const float scale) const
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
            continue;

        // HACK: create a QLabel from the pixmap to apply drop shadows
        QLabel label;

        auto scaledShadow = shadow.scaled(scale / label.devicePixelRatioF());

        // NOTE: avoid scaling issues on high DPI displays
        pixmap.setDevicePixelRatio(label.devicePixelRatioF());

        label.setPixmap(pixmap);

        auto dropShadow = scaledShadow.getGraphicsEffect();
        label.setGraphicsEffect(dropShadow);

        pixmap = label.grab();
        pixmap.setDevicePixelRatio(1);

        label.deleteLater();
        delete dropShadow;
    }

    if (drawColon)
    {
        const auto colonColor =
            getApp()->getThemes()->messages.textColors.regular;

        pixmapPainter.begin(&pixmap);

        pixmapPainter.setPen(QPen(colonColor));
        pixmapPainter.setFont(font);

        const QRectF colonBoundingRect(nametagBoundingRect.right(), 0, 10000,
                                       10000);
        pixmapPainter.drawText(colonBoundingRect, ":",
                               QTextOption(Qt::AlignLeft | Qt::AlignTop));
        pixmapPainter.end();
    }

    return pixmap;
}

QColor Paint::overlayColors(const QColor background,
                            const QColor foreground) const
{
    const auto alpha = foreground.alphaF();

    const auto r = (1 - alpha) * background.red() + alpha * foreground.red();
    const auto g =
        (1 - alpha) * background.green() + alpha * foreground.green();
    const auto b = (1 - alpha) * background.blue() + alpha * foreground.blue();

    return QColor(r, g, b);
}

float Paint::offsetRepeatingStopPosition(const float position,
                                         const QGradientStops stops) const
{
    const float gradientStart = stops.first().first;
    const float gradientEnd = stops.last().first;
    const float gradientLength = gradientEnd - gradientStart;
    const float offsetPosition = (position - gradientStart) / gradientLength;

    return offsetPosition;
}

}  // namespace chatterino
