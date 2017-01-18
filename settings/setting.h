#ifndef SETTING_H
#define SETTING_H

#include <QString>

namespace chatterino {
namespace settings {

class Setting
{
public:
    explicit Setting(const QString &name)
        : name(name)
    {
    }

    const QString &
    getName() const
    {
        return name;
    }

    virtual QString toString() = 0;

private:
    QString name;
};
}

#endif  // SETTING_H
