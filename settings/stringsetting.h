#ifndef STRINGSETTING_H
#define STRINGSETTING_H

#include "settings/setting.h"

#include <QString>

namespace chatterino {
namespace settings {

class StringSetting : public Setting
{
public:
    StringSetting(const QString &name, const QString &defaultValue)
        : Setting(name)
        , value(defaultValue)
        , defaultValue(defaultValue)
    {
    }

    const QString &
    get() const
    {
        return this->value;
    }

    const QString &
    set(const QString &value)
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
    QString value;
    QString defaultValue;
};
}
}

#endif  // STRINGSETTING_H
