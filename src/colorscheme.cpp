#define LOOKUP_COLOR_COUNT 360

#include "colorscheme.hpp"
#include "windowmanager.hpp"

#include <QColor>

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
        return QColor::fromHslF(h, s, (((l - 0.5) * multiplier) + 0.5), a);
    };

    DropPreviewBackground = getColor(hue, 0.5, 0.5, 0.6);

    Text = TextCaret = lightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);

    // tab
    if (hasDarkBorder) {
        //    TabPanelBackground = getColor(hue, 0, 0.8);
        //    TabBackground = getColor(hue, 0, 0.8);
        //    TabHoverBackground = getColor(hue, 0, 0.8);
    } else {
        TabPanelBackground = QColor(255, 255, 255);
        TabBackground = QColor(255, 255, 255);
        TabHoverBackground = getColor(hue, 0, 0.05);
    }
    TabSelectedBackground = getColor(hue, 0.5, 0.5);
    TabHighlightedBackground = getColor(hue, 0.5, 0.2);
    TabNewMessageBackground = QBrush(getColor(hue, 0.5, 0.2), Qt::DiagCrossPattern);
    if (hasDarkBorder) {
        //    TabText = QColor(210, 210, 210);
        //    TabHoverText = QColor(210, 210, 210);
        TabText = QColor(0, 0, 0);
    }
    TabHoverText = QColor(0, 0, 0);
    TabSelectedText = QColor(255, 255, 255);
    TabHighlightedText = QColor(0, 0, 0);

    // Chat
    ChatBackground = getColor(0, 0.1, 1);
    ChatHeaderBackground = getColor(0, 0.1, 0.9);
    ChatHeaderBorder = getColor(0, 0.1, 0.85);
    ChatInputBackground = getColor(0, 0.1, 0.95);
    ChatInputBorder = getColor(0, 0.1, 0.9);

    ScrollbarBG = ChatBackground;

    // generate color lookuptable
    fillLookupTableValues(this->middleLookupTable, 0.000, 0.166, 0.66, 0.5);
    fillLookupTableValues(this->middleLookupTable, 0.166, 0.333, 0.5, 0.55);
    fillLookupTableValues(this->middleLookupTable, 0.333, 0.500, 0.55, 0.45);
    fillLookupTableValues(this->middleLookupTable, 0.500, 0.666, 0.45, 0.80);
    fillLookupTableValues(this->middleLookupTable, 0.666, 0.833, 0.80, 0.61);
    fillLookupTableValues(this->middleLookupTable, 0.833, 1.000, 0.61, 0.66);

    fillLookupTableValues(this->minLookupTable, 0.000, 0.166, 0.33, 0.23);
    fillLookupTableValues(this->minLookupTable, 0.166, 0.333, 0.23, 0.27);
    fillLookupTableValues(this->minLookupTable, 0.333, 0.500, 0.27, 0.23);
    fillLookupTableValues(this->minLookupTable, 0.500, 0.666, 0.23, 0.50);
    fillLookupTableValues(this->minLookupTable, 0.666, 0.833, 0.50, 0.30);
    fillLookupTableValues(this->minLookupTable, 0.833, 1.000, 0.30, 0.33);

    // stylesheet
    InputStyleSheet = "background:" + ChatInputBackground.name() + ";" +
                      "border:" + TabSelectedBackground.name() + ";" + "color:" + Text.name() +
                      ";" + "selection-background-color:" + TabSelectedBackground.name();

    updated();
}

void ColorScheme::fillLookupTableValues(double (&array)[360], double from, double to,
                                        double fromValue, double toValue)
{
    double diff = toValue - fromValue;

    int start = from * LOOKUP_COLOR_COUNT;
    int end = to * LOOKUP_COLOR_COUNT;
    int length = end - start;

    for (int i = 0; i < length; i++) {
        array[start + i] = fromValue + (diff * ((double)i / length));
    }
}

void ColorScheme::normalizeColor(QColor &color)
{
    //    double l = color.lightnessF();
    //    double s = color.saturationF();
    //    double x = this->colorLookupTable[std::max(0, color.hue())];
    //    double newL = (l - 1) * x + 1;

    //    newL = s * newL + (1 - s) * l;

    //    newL = newL > 0.5 ? newL : newL / 2 + 0.25;

    //    color.setHslF(color.hueF(), s, newL);

    double l = color.lightnessF();
    double s = color.saturationF();
    int h = std::max(0, color.hue());
    double x = this->middleLookupTable[h];
    x = s * 0.5 + (1 - s) * x;

    double min = this->minLookupTable[h];
    min = (1 - s) * 0.5 + s * min;

    double newL;

    if (l < x) {
        newL = l * ((x - min) / x) + min;

        //        newL = (l * ((x - min) / x) + min);
        //        newL = (1 - s) * newL + s * l;
    } else {
        newL = l;
    }

    color.setHslF(color.hueF(), s, newL);

    //    double newL = (l - 1) * x + 1;

    //    newL = s * newL + (1 - s) * l;

    //    newL = newL > 0.5 ? newL : newL / 2 + 0.25;

    //    color.setHslF(color.hueF(), s, newL);
}

}  // namespace chatterino
