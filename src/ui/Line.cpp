#include "ui/Line.hpp"

#include <QPainter>

namespace chatterino::ui
{
    void Line::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        painter.setPen(QColor("#999"));

        painter.drawLine(
            0, this->height() / 2, this->width(), this->height() / 2);
    }
}  // namespace chatterino::ui
