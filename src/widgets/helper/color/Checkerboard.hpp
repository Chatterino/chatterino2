#pragma once

#include <QPainter>

namespace chatterino {

void drawCheckerboard(QPainter &painter, QRect rect, int tileSize = 4);
inline void drawCheckerboard(QPainter &painter, QSize size, int tileSize = 4)
{
    drawCheckerboard(painter, {{0, 0}, size}, tileSize);
}

}  // namespace chatterino
