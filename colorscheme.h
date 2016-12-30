#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>

class ColorScheme
{
    static bool IsLightTheme;

    static QColor TooltipBackground;
    static QColor TooltipText;

    static QColor ChatSeperator;
    static QColor ChatBackground;
    static QColor ChatBackgroundHighlighted;
    static QColor ChatBackgroundResub;
    static QColor ChatBackgroundWhisper;
    static QColor ChatInputOuter;
    static QColor ChatInputInner;
    static QColor ChatInputBorder;
    static QColor ChatMessageSeperatorBorder;
    static QColor ChatMessageSeperatorBorderInner;
    static QColor ChatBorder;
    static QColor ChatBorderFocused;

    static QColor Text;
    static QColor TextCaret;
    static QColor TextLink;
    static QColor TextFocused;

    static QColor Menu;
    static QColor MenuBorder;

    static QColor ScrollbarBG;
    static QColor ScrollbarThumb;
    static QColor ScrollbarThumbSelected;
    static QColor ScrollbarArrow;

    static QColor TabPanelBG;
    static QColor TabBG;
    static QColor TabHoverBG;
    static QColor TabSelectedBG;
    static QColor TabHighlightedBG;
    static QColor TabNewMessageBG;
    static QColor TabText;
    static QColor TabHoverText;
    static QColor TabSelectedText;
    static QColor TabHighlightedText;

    static void makeScheme(float hue, float multiplyer);

private:
    ColorScheme() {}
};

#endif // COLORSCHEME_H
