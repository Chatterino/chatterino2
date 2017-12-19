#pragma once

#include <QBrush>
#include <QColor>
#include <boost/signals2.hpp>
#include <pajlada/settings/setting.hpp>

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

    static ColorScheme *instance;

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

    QColor TabText;
    QColor TabBackground;

    QColor TabHoverText;
    QColor TabHoverBackground;

    QColor TabSelectedText;
    QColor TabSelectedBackground;

    QColor TabHighlightedText;
    QColor TabHighlightedBackground;

    QColor TabSelectedUnfocusedText;
    QColor TabSelectedUnfocusedBackground;

    QBrush TabNewMessageBackground;

    QColor Selection;

    const int HighlightColorCount = 3;
    QColor HighlightColors[3];

    void normalizeColor(QColor &color);

    void update();

    boost::signals2::signal<void()> updated;

private:
    pajlada::Settings::Setting<std::string> themeName;
    pajlada::Settings::Setting<double> themeHue;

    void setColors(double hue, double multiplier);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal ratio);

    double middleLookupTable[360] = {};
    double minLookupTable[360] = {};

    void fillLookupTableValues(double (&array)[360], double from, double to, double fromValue,
                               double toValue);

    bool lightTheme = false;
};

}  // namespace chatterino
