#ifndef INTSETTING_H
#define INTSETTING_H

#include "settings/setting.h"

#include <QString>

namespace chatterino {
namespace settings {
class IntSetting : public Setting
{
    Q_OBJECT

public:
    IntSetting(const QString &name, int defaultValue)
        : Setting(name)
        , value(defaultValue)
        , defaultValue(defaultValue)
    {
    }

    int
    get() const
    {
        return this->value;
    }

    int
    set(int value)
    {
        return (this->value = value);
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
    int value;
    int defaultValue;
};
}
}

#endif  // INTSETTING_H
