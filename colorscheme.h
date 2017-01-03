#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <QBrush>

class ColorScheme
{
public:
    bool IsLightTheme;

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

    static ColorScheme& getInstance()
    {
        static ColorScheme instance;

        return instance;
    }

    void setColors(float hue, float multiplyer);

private:
    ColorScheme() {}
};

#endif // COLORSCHEME_H
