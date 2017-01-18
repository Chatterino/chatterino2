#ifndef FONTS_H
#define FONTS_H

#include <QFont>
#include <QFontMetrics>

namespace chatterino {

class Fonts
{
public:
    enum Type : char {
        Medium,
        MediumBold,
        MediumItalic,
        Small,
        Large,
        VeryLarge
    };

    static QFont &getFont(Type type);
    static QFontMetrics &getFontMetrics(Type type);

    static int
    getGeneration()
    {
        return generation;
    }

    static void
    incGeneration()
    {
        generation++;
    }

private:
    Fonts();

    static QFont *medium;
    static QFont *mediumBold;
    static QFont *mediumItalic;
    static QFont *small;
    static QFont *large;
    static QFont *veryLarge;

    static QFontMetrics *metricsMedium;
    static QFontMetrics *metricsMediumBold;
    static QFontMetrics *metricsMediumItalic;
    static QFontMetrics *metricsSmall;
    static QFontMetrics *metricsLarge;
    static QFontMetrics *metricsVeryLarge;

    static int generation;
};
}

#endif  // FONTS_H
