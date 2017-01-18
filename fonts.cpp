#include "fonts.h"

#define DEFAULT_FONT "Arial"

namespace chatterino {

QFont *Fonts::medium = new QFont(DEFAULT_FONT, 14);
QFont *Fonts::mediumBold = new QFont(DEFAULT_FONT, 14);
QFont *Fonts::mediumItalic = new QFont(DEFAULT_FONT, 14);
QFont *Fonts::small = new QFont(DEFAULT_FONT, 10);
QFont *Fonts::large = new QFont(DEFAULT_FONT, 16);
QFont *Fonts::veryLarge = new QFont(DEFAULT_FONT, 18);

QFontMetrics *Fonts::metricsMedium = new QFontMetrics(*medium);
QFontMetrics *Fonts::metricsMediumBold = new QFontMetrics(*mediumBold);
QFontMetrics *Fonts::metricsMediumItalic = new QFontMetrics(*mediumItalic);
QFontMetrics *Fonts::metricsSmall = new QFontMetrics(*small);
QFontMetrics *Fonts::metricsLarge = new QFontMetrics(*large);
QFontMetrics *Fonts::metricsVeryLarge = new QFontMetrics(*veryLarge);

int Fonts::generation = 0;

Fonts::Fonts()
{
}

QFont &
Fonts::getFont(Type type)
{
    if (type == Medium)
        return *medium;
    if (type == MediumBold)
        return *mediumBold;
    if (type == MediumItalic)
        return *mediumItalic;
    if (type == Small)
        return *small;
    if (type == Large)
        return *large;
    if (type == VeryLarge)
        return *veryLarge;

    return *medium;
}

QFontMetrics &
Fonts::getFontMetrics(Type type)
{
    if (type == Medium)
        return *metricsMedium;
    if (type == MediumBold)
        return *metricsMediumBold;
    if (type == MediumItalic)
        return *metricsMediumItalic;
    if (type == Small)
        return *metricsSmall;
    if (type == Large)
        return *metricsLarge;
    if (type == VeryLarge)
        return *metricsVeryLarge;

    return *metricsMedium;
}
}
