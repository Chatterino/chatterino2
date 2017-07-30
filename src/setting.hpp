#pragma once

#include <QSettings>
#include <QString>
#include <boost/signals2.hpp>

namespace chatterino {

class BaseSetting
{
public:
    BaseSetting(const QString &_name)
        : _name(_name)
    {
    }

    virtual QVariant getVariant() = 0;
    virtual void setVariant(QVariant value) = 0;

    const QString &getName() const
    {
        return _name;
    }

private:
    QString _name;
};

template <typename T>
class Setting : public BaseSetting
{
public:
    Setting(std::vector<std::reference_wrapper<BaseSetting>> &settingItems, const QString &_name,
            const T &defaultValue)
        : BaseSetting(_name)
        , _value(defaultValue)
    {
        settingItems.push_back(*this);
    }

    const T &get() const
    {
        return _value;
    }

    T &getnonConst() {
        return _value;
    }

    void set(const T &newValue)
    {
        if (_value != newValue) {
            _value = newValue;

            valueChanged(newValue);
        }
    }

    virtual QVariant getVariant() final
    {
        return QVariant::fromValue(_value);
    }

    virtual void setVariant(QVariant value) final
    {
        if (value.isValid()) {
            assert(value.canConvert<T>());
            set(value.value<T>());
        }
    }

    void insertMap(QString id, bool sound, bool task){
        QPair<bool,bool> pair(sound,task);
        _value.insert(id,pair);
    }


    boost::signals2::signal<void(const T &newValue)> valueChanged;

private:
    T _value;
};

}  // namespace chatterino
