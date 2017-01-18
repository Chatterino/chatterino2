#ifndef REALSETTING_H
#define REALSETTING_H

#include <QString>

namespace chatterino {
namespace settings {

class RealSetting : public Setting
{
public:
    RealSetting(const QString &name, qreal defaultValue,
                qreal minValue = std::numeric_limits<qreal>::min(),
                qreal maxValue = std::numeric_limits<qreal>::max())
        : Setting(name)
        , value(defaultValue)
        , defaultValue(defaultValue)
        , minValue(minValue)
        , maxValue(maxValue)
    {
    }

private:
    qreal value;
    qreal defaultValue;
    qreal minValue;
    qreal maxValue;
};
}
}

#endif  // REALSETTING_H
