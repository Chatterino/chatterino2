#include "util/FuzzyConvert.hpp"

#include <QRegularExpression>

namespace chatterino {

int fuzzyToInt(const QString &str, int default_)
{
    static auto intFinder = QRegularExpression("[0-9]+");

    auto match = intFinder.match(str);
    if (match.hasMatch())
    {
        return match.captured().toInt();
    }

    return default_;
}

float fuzzyToFloat(const QString &str, float default_)
{
    static auto floatFinder = QRegularExpression("[0-9]+(\\.[0-9]+)?");

    auto match = floatFinder.match(str);
    if (match.hasMatch())
    {
        return match.captured().toFloat();
    }

    return default_;
}

}  // namespace chatterino
