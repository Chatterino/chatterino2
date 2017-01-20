#ifndef SETTING_H
#define SETTING_H

#include <QSettings>
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

    virtual void save(const QSettings &settings) = 0;
    virtual void load(const QSettings &settings) = 0;

protected:
    const QString &
    getName() const
    {
        return name;
    }

private:
    QString name;
};
}
}

#endif  // SETTING_H
