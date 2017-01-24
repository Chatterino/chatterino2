#ifndef SETTING_H
#define SETTING_H

#include <QSettings>
#include <QString>
#include <boost/signals2.hpp>

namespace chatterino {

class BaseSetting
{
public:
    BaseSetting(const QString &_name)
        : name(_name)
    {
    }

    virtual QVariant getVariant() = 0;
    virtual void setVariant(QVariant value) = 0;

    const QString &
    getName() const
    {
        return this->name;
    }

private:
    QString name;
};

template <typename T>
class Setting : public BaseSetting
{
public:
    Setting(std::vector<std::reference_wrapper<BaseSetting>> &settingItems,
            const QString &_name, const T &defaultValue)
        : BaseSetting(_name)
        , value(defaultValue)
    {
        settingItems.push_back(*this);
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

    virtual QVariant
    getVariant() final
    {
        return QVariant::fromValue(value);
    }

    virtual void
    setVariant(QVariant value) final
    {
        if (value.isValid()) {
            assert(value.canConvert<T>());
            this->set(value.value<T>());
        }
    }

    boost::signals2::signal<void(const T &newValue)> valueChanged;

private:
    T value;
};

}  // namespace chatterino

#endif  // SETTING_H
