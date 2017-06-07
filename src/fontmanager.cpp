#include "fontmanager.h"

#define DEFAULT_FONT "Arial"

namespace chatterino {

FontManager FontManager::instance;

FontManager::FontManager()
    : _generation(0)
{
    _medium = new QFont(DEFAULT_FONT, 14);
    _mediumBold = new QFont(DEFAULT_FONT, 14);
    _mediumItalic = new QFont(DEFAULT_FONT, 14);
    _small = new QFont(DEFAULT_FONT, 10);
    _large = new QFont(DEFAULT_FONT, 16);
    _veryLarge = new QFont(DEFAULT_FONT, 18);

    _metricsMedium = new QFontMetrics(*_medium);
    _metricsMediumBold = new QFontMetrics(*_mediumBold);
    _metricsMediumItalic = new QFontMetrics(*_mediumItalic);
    _metricsSmall = new QFontMetrics(*_small);
    _metricsLarge = new QFontMetrics(*_large);
    _metricsVeryLarge = new QFontMetrics(*_veryLarge);
}

QFont &FontManager::getFont(Type type)
{
    if (type == Medium)
        return *_medium;
    if (type == MediumBold)
        return *_mediumBold;
    if (type == MediumItalic)
        return *_mediumItalic;
    if (type == Small)
        return *_small;
    if (type == Large)
        return *_large;
    if (type == VeryLarge)
        return *_veryLarge;

    return *_medium;
}

QFontMetrics &FontManager::getFontMetrics(Type type)
{
    if (type == Medium)
        return *_metricsMedium;
    if (type == MediumBold)
        return *_metricsMediumBold;
    if (type == MediumItalic)
        return *_metricsMediumItalic;
    if (type == Small)
        return *_metricsSmall;
    if (type == Large)
        return *_metricsLarge;
    if (type == VeryLarge)
        return *_metricsVeryLarge;

    return *_metricsMedium;
}

}  // namespace chatterino
