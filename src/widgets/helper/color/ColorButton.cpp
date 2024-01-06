#include "widgets/helper/color/ColorButton.hpp"

#include "widgets/helper/color/Checkerboard.hpp"

#include <QPainterPath>

namespace chatterino {

ColorButton::ColorButton(QColor color, QWidget *parent)
    : QAbstractButton(parent)
    , currentColor_(color)
{
    this->setSizePolicy({QSizePolicy::Expanding, QSizePolicy::Expanding});
    this->setMinimumSize({30, 30});
}

QSize ColorButton::sizeHint() const
{
    return {50, 30};
}

void ColorButton::setColor(const QColor &color)
{
    if (this->currentColor_ == color)
    {
        return;
    }

    this->currentColor_ = color;
    this->update();
}

QColor ColorButton::color() const
{
    return this->currentColor_;
}

void ColorButton::resizeEvent(QResizeEvent * /*event*/)
{
    this->checkerboardCacheValid_ = false;
    this->repaint();
}

void ColorButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    auto rect = this->rect();

    if (this->currentColor_.alpha() != 255)
    {
        if (!this->checkerboardCacheValid_)
        {
            QPixmap cache(this->size());
            cache.fill(Qt::transparent);

            QPainter cachePainter(&cache);
            cachePainter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(QRect(1, 1, this->size().width() - 2,
                                      this->size().height() - 2),
                                5, 5);
            cachePainter.setClipPath(path);

            drawCheckerboard(cachePainter, this->size(),
                             std::min(this->height() / 2, 10));
            cachePainter.end();

            this->checkerboardCache_ = std::move(cache);
            this->checkerboardCacheValid_ = true;
        }
        painter.drawPixmap(rect.topLeft(), this->checkerboardCache_);
    }
    painter.setBrush(this->currentColor_);
    painter.setPen({QColor(255, 255, 255, 127), 1});
    painter.drawRoundedRect(rect.x() + 1, rect.y() + 1, rect.width() - 2,
                            rect.height() - 2, 5, 5);
}

}  // namespace chatterino
