#ifndef FONTS_H
#define FONTS_H

#include <QFont>
#include <QFontMetrics>

namespace chatterino {

class FontManager
{
public:
    enum Type : char { Medium, MediumBold, MediumItalic, Small, Large, VeryLarge };

    static FontManager &getInstance()
    {
        return instance;
    }

    QFont &getFont(Type type);
    QFontMetrics &getFontMetrics(Type type);

    int getGeneration()
    {
        return _generation;
    }

    void incGeneration()
    {
        _generation++;
    }

private:
    static FontManager instance;

    FontManager();

    QFont *_medium;
    QFont *_mediumBold;
    QFont *_mediumItalic;
    QFont *_small;
    QFont *_large;
    QFont *_veryLarge;

    QFontMetrics *_metricsMedium;
    QFontMetrics *_metricsMediumBold;
    QFontMetrics *_metricsMediumItalic;
    QFontMetrics *_metricsSmall;
    QFontMetrics *_metricsLarge;
    QFontMetrics *_metricsVeryLarge;

    int _generation;
};
}

#endif  // FONTS_H
