#define LOOKUP_COLOR_COUNT 360

#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "windowmanager.hpp"

#include <QColor>

namespace chatterino {

void ColorScheme::init(WindowManager &windowManager)
{
    static bool initiated = false;

    if (!initiated) {
        initiated = true;
        ColorScheme::getInstance().update();

        SettingsManager::getInstance().theme.valueChanged.connect([](const QString &) {
            ColorScheme::getInstance().update();  //
        });

        SettingsManager::getInstance().themeHue.valueChanged.connect([](const float &) {
            ColorScheme::getInstance().update();  //
        });

        ColorScheme::getInstance().updated.connect([&windowManager] {
            windowManager.repaintVisibleChatWidgets();  //
        });
    }
}

void ColorScheme::update()
{
    QString theme = SettingsManager::getInstance().theme.get();
    theme = theme.toLower();

    qreal hue = SettingsManager::getInstance().themeHue.get();

    if (theme == "light") {
        setColors(hue, 0.8);
    } else if (theme == "white") {
        setColors(hue, 1);
    } else if (theme == "black") {
        setColors(hue, -1);
    } else {
        setColors(hue, -0.8);
    }
}

// hue: theme color (0 - 1)
// multiplyer: 1 = white, 0.8 = light, -0.8 dark, -1 black
void ColorScheme::setColors(float hue, float multiplyer)
{
    IsLightTheme = multiplyer > 0;
    bool hasDarkBorder = false;

    SystemMessageColor = QColor(140, 127, 127);

    auto getColor = [multiplyer](qreal h, qreal s, qreal l, qreal a = 1.0) {
        return QColor::fromHslF(h, s, (((l - 0.5) * multiplyer) + 0.5), a);
    };

    DropPreviewBackground = getColor(hue, 0.5, 0.5, 0.6);

    Text = TextCaret = IsLightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);

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

void ColorScheme::fillLookupTableValues(qreal (&array)[360], qreal from, qreal to, qreal fromValue,
                                        qreal toValue)
{
    qreal diff = toValue - fromValue;

    int start = from * LOOKUP_COLOR_COUNT;
    int end = to * LOOKUP_COLOR_COUNT;
    int length = end - start;

    for (int i = 0; i < length; i++) {
        array[start + i] = fromValue + (diff * ((qreal)i / length));
    }
}

void ColorScheme::normalizeColor(QColor &color)
{
    //    qreal l = color.lightnessF();
    //    qreal s = color.saturationF();
    //    qreal x = this->colorLookupTable[std::max(0, color.hue())];
    //    qreal newL = (l - 1) * x + 1;

    //    newL = s * newL + (1 - s) * l;

    //    newL = newL > 0.5 ? newL : newL / 2 + 0.25;

    //    color.setHslF(color.hueF(), s, newL);

    qreal l = color.lightnessF();
    qreal s = color.saturationF();
    int h = std::max(0, color.hue());
    qreal x = this->middleLookupTable[h];
    x = s * 0.5 + (1 - s) * x;

    qreal min = this->minLookupTable[h];
    min = (1 - s) * 0.5 + s * min;

    qreal newL;

    if (l < x) {
        newL = l * ((x - min) / x) + min;

        //        newL = (l * ((x - min) / x) + min);
        //        newL = (1 - s) * newL + s * l;
    } else {
        newL = l;
    }

    color.setHslF(color.hueF(), s, newL);

    //    qreal newL = (l - 1) * x + 1;

    //    newL = s * newL + (1 - s) * l;

    //    newL = newL > 0.5 ? newL : newL / 2 + 0.25;

    //    color.setHslF(color.hueF(), s, newL);
}

}  // namespace chatterino
