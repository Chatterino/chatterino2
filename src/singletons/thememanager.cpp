#define LOOKUP_COLOR_COUNT 360

#include "thememanager.hpp"

#include <QColor>

#include <math.h>

namespace chatterino {
namespace singletons {

namespace detail {

double getMultiplierByTheme(const std::string &themeName)
{
    if (themeName == "Light") {
        return 0.8;
    } else if (themeName == "White") {
        return 1.0;
    } else if (themeName == "Black") {
        return -1.0;
    } else if (themeName == "Dark") {
        return -0.8;
    }

    return -0.8;
}

}  // namespace detail

ThemeManager &ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : themeName("/appearance/theme/name", "Dark")
    , themeHue("/appearance/theme/hue", 0.0)
{
    this->update();

    this->themeName.connectSimple([this](auto) { this->update(); });
    this->themeHue.connectSimple([this](auto) { this->update(); });
}

void ThemeManager::update()
{
    this->actuallyUpdate(this->themeHue, detail::getMultiplierByTheme(this->themeName));
}

// hue: theme color (0 - 1)
// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void ThemeManager::actuallyUpdate(double hue, double multiplier)
{
    lightTheme = multiplier > 0;

    qreal sat = 0.05;

    SystemMessageColor = QColor(140, 127, 127);

    auto getColor = [multiplier](double h, double s, double l, double a = 1.0) {
        return QColor::fromHslF(h, s, ((l - 0.5) * multiplier) + 0.5, a);
    };

    DropPreviewBackground = getColor(hue, 0.5, 0.5, 0.6);

    Text = TextCaret = lightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);
    TextLink = lightTheme ? QColor(66, 134, 244) : QColor(66, 134, 244);

    // tab
    if (true) {
        TabText = QColor(0, 0, 0);
        TabBackground = QColor(255, 255, 255);

        TabHoverText = QColor(0, 0, 0);
        TabHoverBackground = QColor::fromHslF(hue, 0, 0.95);
    } else {
        // Ubuntu style
        // TODO: add setting for this
        TabText = QColor(210, 210, 210);
        TabBackground = QColor(61, 60, 56);

        TabHoverText = QColor(210, 210, 210);
        TabHoverBackground = QColor(73, 72, 68);
    }

    TabSelectedText = QColor(255, 255, 255);
    TabSelectedBackground = QColor::fromHslF(hue, 0.5, 0.5);

    TabSelectedUnfocusedText = QColor(255, 255, 255);
    TabSelectedUnfocusedBackground = QColor::fromHslF(hue, 0, 0.5);

    TabHighlightedText = QColor(0, 0, 0);
    TabHighlightedBackground = QColor::fromHslF(hue, 0.5, 0.8);

    TabNewMessageBackground = QBrush(QColor::fromHslF(hue, 0.5, 0.8), Qt::DiagCrossPattern);

    // Chat
    bool flat = lightTheme;

    ChatBackground = getColor(0, sat, 1);
    ChatBackgroundHighlighted = blendColors(TabSelectedBackground, ChatBackground, 0.8);
    ChatHeaderBackground = getColor(0, sat, flat ? 1 : 0.9);
    ChatHeaderBorder = getColor(0, sat, flat ? 1 : 0.85);
    ChatInputBackground = getColor(0, sat, flat ? 0.95 : 0.95);
    ChatInputBorder = getColor(0, sat, flat ? 1 : 1);
    ChatSeperator = lightTheme ? QColor(127, 127, 127) : QColor(80, 80, 80);

    // Scrollbar
    ScrollbarBG = getColor(0, sat, 0.94);
    ScrollbarThumb = getColor(0, sat, 0.80);
    ScrollbarThumbSelected = getColor(0, sat, 0.7);
    ScrollbarArrow = getColor(0, sat, 0.9);

    // stylesheet
    InputStyleSheet = "background:" + ChatInputBackground.name() + ";" +
                      "border:" + TabSelectedBackground.name() + ";" + "color:" + Text.name() +
                      ";" + "selection-background-color:" + TabSelectedBackground.name();

    // Selection
    Selection = isLightTheme() ? QColor(0, 0, 0, 64) : QColor(255, 255, 255, 64);

    this->updated();
}

QColor ThemeManager::blendColors(const QColor &color1, const QColor &color2, qreal ratio)
{
    int r = color1.red() * (1 - ratio) + color2.red() * ratio;
    int g = color1.green() * (1 - ratio) + color2.green() * ratio;
    int b = color1.blue() * (1 - ratio) + color2.blue() * ratio;

    return QColor(r, g, b, 255);
}

void ThemeManager::normalizeColor(QColor &color)
{
    if (this->lightTheme) {
        if (color.lightnessF() > 0.5f) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5f);
        }

        if (color.lightnessF() > 0.4f && color.hueF() > 0.1 && color.hueF() < 0.33333) {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() -
                              sin((color.hueF() - 0.1) / (0.3333 - 0.1) * 3.14159) *
                                  color.saturationF() * 0.2);
        }
    } else {
        if (color.lightnessF() < 0.5f) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5f);
        }

        if (color.lightnessF() < 0.6f && color.hueF() > 0.54444 && color.hueF() < 0.83333) {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() +
                              sin((color.hueF() - 0.54444) / (0.8333 - 0.54444) * 3.14159) *
                                  color.saturationF() * 0.4);
        }
    }
}

}  // namespace chatterino
}
