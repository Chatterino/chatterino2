#define LOOKUP_COLOR_COUNT 360

#include "colorscheme.h"

#include <QColor>

// hue: theme color (0 - 1)
// multiplyer: 1 = white, 0.8 = light, -0.8 dark, -1 black
void
ColorScheme::setColors(float hue, float multiplyer)
{
    IsLightTheme = multiplyer > 0;

    SystemMessageColor = QColor(140, 127, 127);

    auto isLightTheme = IsLightTheme;

    auto getColor = [isLightTheme, multiplyer](qreal h, qreal s, qreal l,
                                               qreal a = 1.0) {
        return QColor::fromHslF(h, s, (((l - 0.5) * multiplyer) + 0.5), a);
    };

    DropPreviewBackground = getColor(hue, 0.5, 0.5, 0.3);

    Text = TextCaret = IsLightTheme ? QColor(0, 0, 0) : QColor(255, 255, 255);

    // tab
    TabPanelBackground = QColor(255, 255, 255);
    TabBackground = QColor(255, 255, 255);
    TabHoverBackground = getColor(hue, 0, 0.05);
    TabSelectedBackground = getColor(hue, 0.5, 0.5);
    TabHighlightedBackground = getColor(hue, 0.5, 0.2);
    TabNewMessageBackground =
        QBrush(getColor(hue, 0.5, 0.2), Qt::DiagCrossPattern);
    TabText = QColor(0, 0, 0);
    TabHoverText = QColor(0, 0, 0);
    TabSelectedText = QColor(255, 255, 255);
    TabHighlightedText = QColor(0, 0, 0);

    // Chat
    ChatBackground = getColor(0, 0.1, 1);
    ChatHeaderBackground = getColor(0, 0.1, 0.9);
    ChatHeaderBorder = getColor(0, 0.1, 0.85);
    ChatInputBackground = getColor(0, 0.1, 0.95);
    ChatInputBorder = getColor(0, 0.1, 0.9);

    // generate color lookuptable
    //    fillLookupTableValues(0, 0.1, 0.6, 1);
    //    fillLookupTableValues(0.1, 0.50, 1, 1);
    //    fillLookupTableValues(0.50, 0.65, 1, 0.5);
    //    fillLookupTableValues(0.65, 0.83, 0.5, 0.7);
    //    fillLookupTableValues(0.83, 1, 0.7, 0.6);

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

    //    for (int i = 0; i < LOOKUP_COLOR_COUNT; i++) {
    //        qInfo(QString::number(this->middleLookupTable[i]).toStdString().c_str());
    //    }
}

void ColorScheme::fillLookupTableValues(qreal (&array)[360], qreal from,
                                        qreal to, qreal fromValue,
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

void
ColorScheme::normalizeColor(QColor &color)
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
