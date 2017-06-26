#pragma once

#include <QBrush>
#include <QColor>
#include <boost/signals2.hpp>

namespace chatterino {

class WindowManager;

class ColorScheme
{
public:
    explicit ColorScheme(WindowManager &windowManager);

    inline bool isLightTheme() const
    {
        return this->lightTheme;
    }

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

    void init(WindowManager &windowManager);
    void normalizeColor(QColor &color);

    void update();

    boost::signals2::signal<void()> updated;

private:
    void setColors(double hue, double multiplier);

    double middleLookupTable[360] = {};
    double minLookupTable[360] = {};

    void fillLookupTableValues(double (&array)[360], double from, double to, double fromValue,
                               double toValue);

    bool lightTheme;
};

}  // namespace chatterino
