#define LOOKUP_COLOR_COUNT 360

#include "thememanager.hpp"

#include <QColor>

#include <math.h>

namespace chatterino {
namespace singletons {

namespace detail {

double getMultiplierByTheme(const QString &themeName)
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
    this->actuallyUpdate(this->themeHue, detail::getMultiplierByTheme(this->themeName.getValue()));
}

// hue: theme color (0 - 1)
// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void ThemeManager::actuallyUpdate(double hue, double multiplier)
{
    isLight = multiplier > 0;
    bool isLightTabs;

    QColor themeColor = QColor::fromHslF(hue, 0.5, 0.5);
    QColor themeColorNoSat = QColor::fromHslF(hue, 0, 0.5);

    qreal sat = 0;
    //    0.05;

    auto getColor = [multiplier](double h, double s, double l, double a = 1.0) {
        return QColor::fromHslF(h, s, ((l - 0.5) * multiplier) + 0.5, a);
    };

    //#ifdef USEWINSDK
    //    isLightTabs = isLight;
    //    QColor tabFg = isLight ? "#000" : "#fff";
    //    this->windowBg = isLight ? "#fff" : getColor(0, sat, 0.9);
    //#else
    isLightTabs = true;
    QColor tabFg = isLightTabs ? "#000" : "#fff";
    this->windowBg = "#fff";
    //#endif

    // Ubuntu style
    // TODO: add setting for this
    //        TabText = QColor(210, 210, 210);
    //        TabBackground = QColor(61, 60, 56);
    //        TabHoverText = QColor(210, 210, 210);
    //        TabHoverBackground = QColor(73, 72, 68);

    // message (referenced later)
    this->messages.textColors.caret =  //
        this->messages.textColors.regular = isLight ? "#000" : "#fff";

    // tabs
    // text, {regular, hover, unfocused}
    this->windowBg = "#ccc";

    this->tabs.border = "#999";
    this->tabs.regular = {tabFg, {windowBg, blendColors(windowBg, "#999", 0.5), windowBg}};

    this->tabs.selected = {"#fff", {themeColor, themeColor, QColor::fromHslF(hue, 0, 0.5)}};

    this->tabs.newMessage = {
        tabFg,
        {QBrush(blendColors(themeColor, windowBg, 0.7), Qt::FDiagPattern),
         QBrush(blendColors(themeColor, windowBg, 0.5), Qt::FDiagPattern),
         QBrush(blendColors(themeColorNoSat, windowBg, 0.7), Qt::FDiagPattern)}};

    this->tabs.highlighted = {
        tabFg,
        {blendColors(themeColor, windowBg, 0.7), blendColors(themeColor, windowBg, 0.5),
         blendColors(themeColorNoSat, windowBg, 0.7)}};

    this->windowBg = "#fff";

    // Split
    bool flat = isLight;

    this->splits.messageSeperator = isLight ? QColor(127, 127, 127) : QColor(80, 80, 80);
    this->splits.background = getColor(0, sat, 1);
    this->splits.dropPreview = getColor(hue, 0.5, 0.5, 0.6);
    // this->splits.border
    // this->splits.borderFocused

    this->splits.header.background = getColor(0, sat, flat ? 1 : 0.9);
    this->splits.header.border = getColor(0, sat, flat ? 1 : 0.85);
    this->splits.header.text = this->messages.textColors.regular;

    this->splits.input.background = getColor(0, sat, flat ? 0.95 : 0.95);
    this->splits.input.border = getColor(0, sat, flat ? 1 : 1);
    this->splits.input.text = this->messages.textColors.regular;
    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.color().name() + ";" +
        "color:" + this->messages.textColors.regular.name() + ";" +
        "selection-background-color:" + this->tabs.selected.backgrounds.regular.color().name();

    // Message
    this->messages.textColors.link = isLight ? QColor(66, 134, 244) : QColor(66, 134, 244);
    this->messages.textColors.system = QColor(140, 127, 127);

    this->messages.backgrounds.regular = splits.background;
    this->messages.backgrounds.highlighted = blendColors(
        this->tabs.selected.backgrounds.regular.color(), this->messages.backgrounds.regular, 0.8);
    // this->messages.backgrounds.resub
    // this->messages.backgrounds.whisper
    this->messages.disabled = getColor(0, sat, 1, 0.6);
    // this->messages.seperator =
    // this->messages.seperatorInner =

    // Scrollbar
    this->scrollbars.background = getColor(0, sat, 0.94);
    this->scrollbars.thumb = getColor(0, sat, 0.80);
    this->scrollbars.thumbSelected = getColor(0, sat, 0.7);

    // tooltip
    this->tooltip.background = QColor(0, 0, 0);
    this->tooltip.text = QColor(255, 255, 255);

    // Selection
    this->messages.selection = isLightTheme() ? QColor(0, 0, 0, 64) : QColor(255, 255, 255, 64);

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
    if (this->isLight) {
        if (color.lightnessF() > 0.5f) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5f);
        }

        if (color.lightnessF() > 0.4f && color.hueF() > 0.1 && color.hueF() < 0.33333) {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() - sin((color.hueF() - 0.1) / (0.3333 - 0.1) * 3.14159) *
                                         color.saturationF() * 0.2);
        }
    } else {
        if (color.lightnessF() < 0.5f) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5f);
        }

        if (color.lightnessF() < 0.6f && color.hueF() > 0.54444 && color.hueF() < 0.83333) {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() + sin((color.hueF() - 0.54444) / (0.8333 - 0.54444) * 3.14159) *
                                         color.saturationF() * 0.4);
        }
    }
}

}  // namespace singletons
}  // namespace chatterino
