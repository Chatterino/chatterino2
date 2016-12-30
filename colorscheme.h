#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>

class ColorScheme
{
public:
    bool IsLightTheme;

    QColor TooltipBackground;
    QColor TooltipText;
    QColor ChatSeperator;
    QColor ChatBackground;
    QColor ChatBackgroundHighlighted;
    QColor ChatBackgroundResub;
    QColor ChatBackgroundWhisper;
    QColor ChatInputOuter;
    QColor ChatInputInner;
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
    QColor TabPanelBG;
    QColor TabBG;
    QColor TabHoverBG;
    QColor TabSelectedBG;
    QColor TabHighlightedBG;
    QColor TabNewMessageBG;
    QColor TabText;
    QColor TabHoverText;
    QColor TabSelectedText;
    QColor TabHighlightedText;

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
