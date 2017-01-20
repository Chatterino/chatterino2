#ifndef BOOLSETTING_H
#define BOOLSETTING_H

#include "settings/setting.h"

#include <QString>

namespace chatterino {
namespace settings {
class BoolSetting : public Setting
{
public:
    BoolSetting(const QString &name, bool defaultValue)
        : Setting(name)
        , value(defaultValue)
        , defaultValue(defaultValue)
    {
    }

    bool
    get() const
    {
        return this->value;
    }

    void
    set(bool value)
    {
        this->value = value;
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
    bool value;
    bool defaultValue;
};
}
}

#endif  // BOOLSETTING_H
