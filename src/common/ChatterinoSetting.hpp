#pragma once

#include "util/QMagicEnum.hpp"

#include <pajlada/settings.hpp>
#include <QSize>
#include <QString>

#include <memory>
#include <string>
#include <type_traits>

namespace chatterino {

void _registerSetting(std::weak_ptr<pajlada::Settings::SettingData> setting);

template <typename Type>
class ChatterinoSetting : public pajlada::Settings::Setting<Type>
{
public:
    ChatterinoSetting(const std::string &path)
        : pajlada::Settings::Setting<Type>(
              path, pajlada::Settings::SettingOption::CompareBeforeSet)
    {
        _registerSetting(this->getData());
    }

    ChatterinoSetting(const std::string &path, const Type &defaultValue)
        : pajlada::Settings::Setting<Type>(
              path, defaultValue,
              pajlada::Settings::SettingOption::CompareBeforeSet)
    {
        _registerSetting(this->getData());
    }

    template <typename T2>
    ChatterinoSetting &operator=(const T2 &newValue)
    {
        this->setValue(newValue);

        return *this;
    }

    ChatterinoSetting &operator=(Type &&newValue) noexcept
    {
        pajlada::Settings::Setting<Type>::operator=(newValue);

        return *this;
    }

    using pajlada::Settings::Setting<Type>::operator==;
    using pajlada::Settings::Setting<Type>::operator!=;

    using pajlada::Settings::Setting<Type>::operator Type;
};

using BoolSetting = ChatterinoSetting<bool>;
using FloatSetting = ChatterinoSetting<float>;
using DoubleSetting = ChatterinoSetting<double>;
using IntSetting = ChatterinoSetting<int>;
using StringSetting = ChatterinoSetting<std::string>;
using QStringSetting = ChatterinoSetting<QString>;
using QSizeSetting = ChatterinoSetting<QSize>;

template <typename Enum>
class EnumSetting
    : public ChatterinoSetting<typename std::underlying_type<Enum>::type>
{
    using Underlying = typename std::underlying_type<Enum>::type;

public:
    using ChatterinoSetting<Underlying>::ChatterinoSetting;

    EnumSetting(const std::string &path, const Enum &defaultValue)
        : ChatterinoSetting<Underlying>(path, Underlying(defaultValue))
    {
        _registerSetting(this->getData());
    }

    EnumSetting<Enum> &operator=(Enum newValue)
    {
        this->setValue(Underlying(newValue));

        return *this;
    }

    operator Enum()
    {
        return Enum(this->getValue());
    }

    Enum getEnum()
    {
        return Enum(this->getValue());
    }
};

/**
 * Setters in this class allow for bad values, it's only the enum-specific getters that are protected.
 * If you get a QString from this setting, it will be the raw value from the settings file.
 * Use the explicit Enum conversions or getEnum to get a typed check with a default
 **/
template <typename Enum>
class EnumStringSetting : public pajlada::Settings::Setting<QString>
{
public:
    EnumStringSetting(const std::string &path, const Enum &defaultValue_)
        : pajlada::Settings::Setting<QString>(path)
        , defaultValue(defaultValue_)
    {
        _registerSetting(this->getData());
    }

    template <typename T2>
    EnumStringSetting<Enum> &operator=(Enum newValue)
    {
        this->setValue(qmagicenum::enumNameString(newValue).toLower());

        return *this;
    }

    EnumStringSetting<Enum> &operator=(QString newValue)
    {
        this->setValue(newValue.toLower());

        return *this;
    }

    operator Enum()
    {
        return this->getEnum();
    }

    Enum getEnum()
    {
        return qmagicenum::enumCast<Enum>(this->getValue(),
                                          qmagicenum::CASE_INSENSITIVE)
            .value_or(this->defaultValue);
    }

    Enum defaultValue;

    using pajlada::Settings::Setting<QString>::operator==;
    using pajlada::Settings::Setting<QString>::operator!=;

    using pajlada::Settings::Setting<QString>::operator QString;
};

template <typename T>
struct IsChatterinoSettingT : std::false_type {
};
template <typename T>
struct IsChatterinoSettingT<ChatterinoSetting<T>> : std::true_type {
};
template <typename T>
struct IsChatterinoSettingT<EnumStringSetting<T>> : std::true_type {
};

template <typename T>
concept IsChatterinoSetting = IsChatterinoSettingT<T>::value;

}  // namespace chatterino
