#pragma once

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

    const QString &getName() const
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
    Setting(std::vector<std::reference_wrapper<BaseSetting>> &settingItems, const QString &_name,
            const T &defaultValue)
        : BaseSetting(_name)
        , value(defaultValue)
    {
        settingItems.push_back(*this);
    }

    const T &get() const
    {
        return this->value;
    }

    T &getnonConst()
    {
        return this->value;
    }

    void set(const T &newValue)
    {
        if (this->value != newValue) {
            this->value = newValue;

            valueChanged(newValue);
        }
    }

    virtual QVariant getVariant() final
    {
        return QVariant::fromValue(this->value);
    }

    virtual void setVariant(QVariant newValue) final
    {
        if (newValue.isValid()) {
            assert(newValue.canConvert<T>());
            set(newValue.value<T>());
        }
    }

    void insertMap(QString id, bool sound, bool task)
    {
        QPair<bool, bool> pair(sound, task);
        this->value.insert(id, pair);
    }

    boost::signals2::signal<void(const T &newValue)> valueChanged;

private:
    T value;
};

}  // namespace chatterino
