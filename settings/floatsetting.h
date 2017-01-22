#ifndef REALSETTING_H
#define REALSETTING_H

#include "settings/setting.h"

#include <QString>

namespace chatterino {
namespace settings {

class FloatSetting : public Setting
{
    Q_OBJECT

public:
    FloatSetting(const QString &name, qreal defaultValue,
                 qreal minValue = std::numeric_limits<qreal>::min(),
                 qreal maxValue = std::numeric_limits<qreal>::max())
        : Setting(name)
        , value(defaultValue)
        , defaultValue(defaultValue)
        , minValue(minValue)
        , maxValue(maxValue)
    {
    }

    qreal
    get() const
    {
        return this->value;
    }

    qreal
    set(qreal value)
    {
        return (this->value = std::max(std::min(value, maxValue), minValue));
    }

    void
    save(const QSettings &settings) override
    {
    }

    void
    load(const QSettings &settings) override
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
