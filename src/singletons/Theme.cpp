#define LOOKUP_COLOR_COUNT 360

#include "singletons/Theme.hpp"
#include "Application.hpp"

#include <QColor>

#include <cmath>

namespace chatterino {

Theme::Theme()
{
    this->update();

    this->themeName.connectSimple([this](auto) { this->update(); }, false);
    this->themeHue.connectSimple([this](auto) { this->update(); }, false);
}

// hue: theme color (0 - 1)
// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void Theme::actuallyUpdate(double hue, double multiplier)
{
    ABTheme::actuallyUpdate(hue, multiplier);

    auto getColor = [multiplier](double h, double s, double l, double a = 1.0) {
        return QColor::fromHslF(h, s, ((l - 0.5) * multiplier) + 0.5, a);
    };

    auto sat = qreal(0);
    auto isLight_ = this->isLightTheme();
    auto flat = isLight_;

    if (this->isLightTheme())
    {
        this->splits.dropTargetRect = QColor(255, 255, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0xff);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x50);
    }
    else
    {
        this->splits.dropTargetRect = QColor(0, 148, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0x70);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x20);
    }

    this->splits.header.background = getColor(0, sat, flat ? 1 : 0.9);
    this->splits.header.border = getColor(0, sat, flat ? 1 : 0.85);
    this->splits.header.text = this->messages.textColors.regular;
    this->splits.header.focusedText =
        isLight_ ? QColor("#198CFF") : QColor("#84C1FF");

    this->splits.input.background = getColor(0, sat, flat ? 0.95 : 0.95);
    this->splits.input.border = getColor(0, sat, flat ? 1 : 1);
    this->splits.input.text = this->messages.textColors.regular;
    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.color().name() +
        ";" + "color:" + this->messages.textColors.regular.name() + ";" +  //
        "selection-background-color:" +
        (isLight_ ? "#68B1FF"
                  : this->tabs.selected.backgrounds.regular.color().name());

    this->splits.input.focusedLine = this->tabs.highlighted.line.regular;

    this->splits.messageSeperator =
        isLight_ ? QColor(127, 127, 127) : QColor(60, 60, 60);
    this->splits.background = getColor(0, sat, 1);
    this->splits.dropPreview = QColor(0, 148, 255, 0x30);
    this->splits.dropPreviewBorder = QColor(0, 148, 255, 0xff);
}

void Theme::normalizeColor(QColor &color)
{
    if (this->isLightTheme())
    {
        if (color.lightnessF() > 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() > 0.4 && color.hueF() > 0.1 &&
            color.hueF() < 0.33333)
        {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() - sin((color.hueF() - 0.1) /
                                                   (0.3333 - 0.1) * 3.14159) *
                                                   color.saturationF() * 0.4);
        }
    }
    else
    {
        if (color.lightnessF() < 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() < 0.6 && color.hueF() > 0.54444 &&
            color.hueF() < 0.83333)
        {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() + sin((color.hueF() - 0.54444) /
                                         (0.8333 - 0.54444) * 3.14159) *
                                         color.saturationF() * 0.4);
        }
    }
}

Theme *getTheme()
{
    return getApp()->themes;
}

}  // namespace chatterino
