#include "colorscheme.h"
#include "QColor"

// hue: theme color (0 - 1)
// multiplyer: 1 = white, 0.8 = light, -0.8 dark, -1 black
void
ColorScheme::setColors(float hue, float multiplyer)
{
    IsLightTheme = multiplyer > 0;

    SystemMessageColor = QColor(140, 127, 127);

    auto isLightTheme = IsLightTheme;

    auto getColor = [isLightTheme, multiplyer](qreal h, qreal s, qreal l,
                                               qreal a = 1.0) {
        return QColor::fromHslF(h, s, (((l - 0.5) * multiplyer) + 0.5), a);
    };

    DropPreviewBackground = getColor(hue, 0.5, 0.5, 0.3);

    Text = TextCaret = IsLightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);

    // tab
    TabPanelBackground = QColor(255, 255, 255);
    TabBackground = QColor(255, 255, 255);
    TabHoverBackground = getColor(hue, 0, 0.05);
    TabSelectedBackground = getColor(hue, 0.5, 0.5);
    TabHighlightedBackground = getColor(hue, 0.5, 0.2);
    TabNewMessageBackground =
        QBrush(getColor(hue, 0.5, 0.2), Qt::DiagCrossPattern);
    TabText = QColor(0, 0, 0);
    TabHoverText = QColor(0, 0, 0);
    TabSelectedText = QColor(255, 255, 255);
    TabHighlightedText = QColor(0, 0, 0);

    // Chat
    ChatBackground = getColor(0, 0.1, 1);
    ChatHeaderBackground = getColor(0, 0.1, 0.9);
    ChatHeaderBorder = getColor(0, 0.1, 0.85);
    ChatInputBackground = getColor(0, 0.1, 0.95);
    ChatInputBorder = getColor(0, 0.1, 0.9);
}
