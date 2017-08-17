#define LOOKUP_COLOR_COUNT 360

#include "colorscheme.hpp"
#include "windowmanager.hpp"

#include <QColor>

#include <math.h>

namespace chatterino {

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

ColorScheme::ColorScheme(WindowManager &windowManager)
    : themeName("/appearance/theme/name", "Dark")
    , themeHue("/appearance/theme/hue", 0.0)
{
    this->update();

    this->themeName.getValueChangedSignal().connect([=](const auto &) {
        this->update();  //
    });

    this->themeHue.getValueChangedSignal().connect([=](const auto &) {
        this->update();  //
    });

    this->updated.connect([&windowManager] {
        windowManager.repaintVisibleChatWidgets();  //
    });
}

void ColorScheme::update()
{
    this->setColors(this->themeHue, detail::getMultiplierByTheme(this->themeName));
}

// hue: theme color (0 - 1)
// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void ColorScheme::setColors(double hue, double multiplier)
{
    lightTheme = multiplier > 0;
    bool hasDarkBorder = false;

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
        TabHoverBackground = getColor(hue, 0, 0.05);
    } else {
        // Ubuntu style
        // TODO: add setting for this
        TabText = QColor(210, 210, 210);
        TabBackground = QColor(61, 60, 56);

        TabHoverText = QColor(210, 210, 210);
        TabHoverBackground = QColor(73, 72, 68);
    }

    TabSelectedText = QColor(255, 255, 255);
    TabSelectedBackground = getColor(hue, 0.5, 0.5);

    TabHighlightedText = QColor(0, 0, 0);
    TabHighlightedBackground = getColor(hue, 0.5, 0.2);

    TabNewMessageBackground = QBrush(getColor(hue, 0.5, 0.2), Qt::DiagCrossPattern);

    // Chat
    ChatBackground = getColor(0, 0.1, 1);
    ChatHeaderBackground = getColor(0, 0.1, 0.9);
    ChatHeaderBorder = getColor(0, 0.1, 0.85);
    ChatInputBackground = getColor(0, 0.1, 0.95);
    ChatInputBorder = getColor(0, 0.1, 0.9);
    ChatSeperator = lightTheme ? QColor(127, 127, 127) : QColor(80, 80, 80);

    // Scrollbar
    ScrollbarBG = ChatBackground;
    ScrollbarThumb = getColor(0, 0.1, 0.85);
    ScrollbarThumbSelected = getColor(0, 0.1, 0.7);
    ScrollbarArrow = getColor(0, 0.1, 0.4);

    // stylesheet
    InputStyleSheet = "background:" + ChatInputBackground.name() + ";" +
                      "border:" + TabSelectedBackground.name() + ";" + "color:" + Text.name() +
                      ";" + "selection-background-color:" + TabSelectedBackground.name();

    this->updated();
}

void ColorScheme::normalizeColor(QColor &color)
{
    if (this->lightTheme) {
        // TODO: write some codes
    } else {
        if (color.lightnessF() < 0.5f) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5f);
        }

        if (color.lightnessF() < 0.6f && color.hueF() > 0.54444 && color.hueF() < 0.83333) {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() +
                              sin((color.hueF() - 0.54444) / (0.8333 - 0.54444) * 3.14159) *
                                  color.saturationF() * 0.2);
        }
    }
}

}  // namespace chatterino
