#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QBrush>
#include <QColor>
#include <boost/signals2.hpp>

namespace chatterino {

class ColorScheme
{
public:
    bool IsLightTheme;

    QString InputStyleSheet;

    QColor SystemMessageColor;

    QColor DropPreviewBackground;

    QColor TooltipBackground;
    QColor TooltipText;
    QColor ChatSeperator;
    QColor ChatBackground;
    QColor ChatBackgroundHighlighted;
    QColor ChatBackgroundResub;
    QColor ChatBackgroundWhisper;

    QColor ChatHeaderBorder;
    QColor ChatHeaderBackground;

    QColor ChatInputBackground;
    QColor ChatInputBorder;

    QColor ChatMessageSeperatorBorder;
    QColor ChatMessageSeperatorBorderInner;
    QColor ChatBorder;
    QColor ChatBorderFocused;
    QColor Text;
    QColor TextCaret;
    QColor TextLink;
    QColor TextFocused;
    QColor Menu;
    QColor MenuBorder;
    QColor ScrollbarBG;
    QColor ScrollbarThumb;
    QColor ScrollbarThumbSelected;
    QColor ScrollbarArrow;

    QColor TabPanelBackground;
    QColor TabBackground;
    QColor TabHoverBackground;
    QColor TabSelectedBackground;
    QColor TabHighlightedBackground;
    QBrush TabNewMessageBackground;
    QColor TabText;
    QColor TabHoverText;
    QColor TabSelectedText;
    QColor TabHighlightedText;

    const int HighlightColorCount = 3;
    QColor HighlightColors[3];

    static ColorScheme &
    getInstance()
    {
        static ColorScheme instance;

        return instance;
    }

    void init();
    void normalizeColor(QColor &color);

    void update();

    boost::signals2::signal<void()> updated;

private:
    ColorScheme()
        : updated()
    {
    }

    void setColors(float hue, float multiplyer);

    qreal middleLookupTable[360] = {};
    qreal minLookupTable[360] = {};

    void fillLookupTableValues(qreal (&array)[360], qreal from, qreal to,
                               qreal fromValue, qreal toValue);
};
}

#endif  // COLORSCHEME_H
