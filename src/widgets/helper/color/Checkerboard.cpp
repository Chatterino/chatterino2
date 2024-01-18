#include "widgets/helper/color/Checkerboard.hpp"

namespace chatterino {

void drawCheckerboard(QPainter &painter, QRect rect, int tileSize)
{
    painter.fillRect(rect, QColor(255, 255, 255));

    if (tileSize <= 0)
    {
        tileSize = 1;
    }

    int overflowY = rect.height() % tileSize == 0 ? 0 : 1;
    int overflowX = rect.width() % tileSize == 0 ? 0 : 1;
    for (int row = 0; row < rect.height() / tileSize + overflowY; row++)
    {
        int offsetX = row % 2 == 0 ? 0 : 1;
        for (int col = offsetX; col < rect.width() / tileSize + overflowX;
             col += 2)
        {
            painter.fillRect(rect.x() + col * tileSize,
                             rect.y() + row * tileSize, tileSize, tileSize,
                             QColor(204, 204, 204));
        }
    }
}

}  // namespace chatterino
