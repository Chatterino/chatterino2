// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/buttons/PixmapButton.hpp"

namespace {

/**
 * Resizes a pixmap to a desired size.
 * Does nothing if the target pixmap is already sized correctly.
 * 
 * @param target The target pixmap.
 * @param source The unscaled pixmap.
 * @param size The desired device independent size.
 * @param dpr The device pixel ratio of the target area. The size of the target in pixels will be `size * dpr`.
 */
void resizePixmap(QPixmap &target, const QPixmap &source, const QSize &size,
                  qreal dpr)
{
    if (target.deviceIndependentSize() == size)
    {
        return;
    }

    QPixmap resized = source;
    resized.setDevicePixelRatio(dpr);
    target = resized.scaled(size * dpr, Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
}

}  // namespace

namespace chatterino {

PixmapButton::PixmapButton(BaseWidget *parent)
    : DimButton(parent)
{
}

QPixmap PixmapButton::pixmap() const
{
    return this->pixmap_;
}

void PixmapButton::setPixmap(const QPixmap &pixmap)
{
    // Avoid updates if the pixmap didn't change
    if (pixmap.cacheKey() == this->pixmap_.cacheKey())
    {
        return;
    }

    this->pixmap_ = pixmap;
    this->resizedPixmap_ = {};
    this->update();
}

bool PixmapButton::marginEnabled() const noexcept
{
    return this->marginEnabled_;
}

void PixmapButton::setMarginEnabled(bool enableMargin)
{
    this->marginEnabled_ = enableMargin;
    this->update();
}

void PixmapButton::paintContent(QPainter &painter)
{
    if (this->pixmap_.isNull())
    {
        return;
    }
    painter.setOpacity(this->currentContentOpacity());

    QRect rect = this->rect();

    int shift = 0;
    if (this->marginEnabled_)
    {
        int margin =
            this->height() < static_cast<int>(22 * this->scale()) ? 3 : 6;

        shift = static_cast<int>(static_cast<float>(margin) * this->scale());
    }

    rect.moveLeft(shift);
    rect.setRight(rect.right() - (2 * shift));
    rect.moveTop(shift);
    rect.setBottom(rect.bottom() - (2 * shift));

    resizePixmap(this->resizedPixmap_, this->pixmap_, rect.size(),
                 this->devicePixelRatio());

    painter.drawPixmap(rect, this->resizedPixmap_);

    painter.setOpacity(1);
}

}  // namespace chatterino
