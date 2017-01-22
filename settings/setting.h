#ifndef SETTING_H
#define SETTING_H

#include <QSettings>
#include <QString>
#include <boost/signals2.hpp>

namespace chatterino {
namespace settings {

class BaseSetting
{
public:
    virtual void save(QSettings &settings) = 0;
    virtual void load(const QSettings &settings) = 0;
};

template <typename T>
class Setting : public BaseSetting
{
public:
    Setting(const QString &_name, const T &defaultValue)
        : name(_name)
        , value(defaultValue)
    {
    }

    const T &
    get() const
    {
        return this->value;
    }

    void
    set(const T &newValue)
    {
        if (this->value != newValue) {
            this->value = newValue;

            this->valueChanged(newValue);
        }
    }

    virtual void
    save(QSettings &settings) final
    {
        settings.setValue(this->getName(), QVariant::fromValue(this->value));
    }

    virtual void
    load(const QSettings &settings) final
    {
        QVariant newValue = settings.value(this->getName(), QVariant());
        if (newValue.isValid()) {
            assert(newValue.canConvert<T>());
            this->value = newValue.value<T>();
        }
    }

    boost::signals2::signal<void(const T &newValue)> valueChanged;

protected:
    const QString &
    getName() const
    {
        return name;
    }

private:
    QString name;
    T value;
};

}  // namespace settings
}  // namespace chatterino

#endif  // SETTING_H
