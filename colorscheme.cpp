#include "QColor"
#include "colorscheme.h"

// hue: theme color (0 - 1)
// multiplyer: 1 = white, 0.8 = light, -0.8 dark, -1 black
void ColorScheme::setColors(float hue, float multiplyer)
{
    IsLightTheme = multiplyer > 0;

    auto isLightTheme = IsLightTheme;

    auto getColor = [isLightTheme, multiplyer] (qreal h, qreal s, qreal l) -> QColor
    {
        return QColor::fromHslF(h, s, (((l - 0.5) * multiplyer) + 0.5));
    };

    TextCaret = IsLightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);

//    ChatBorder = IsLightTheme ? QColor()
}
