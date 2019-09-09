#pragma once

#include <QString>
#include <pajlada/settings.hpp>

namespace AB_NAMESPACE {

void _registerSetting(std::weak_ptr<pajlada::Settings::SettingData> setting);

template <typename Type>
class ChatterinoSetting : public pajlada::Settings::Setting<Type>
{
public:
    ChatterinoSetting(const std::string &path)
        : pajlada::Settings::Setting<Type>(path)
    {
        _registerSetting(this->getData());
    }

    ChatterinoSetting(const std::string &path, const Type &defaultValue)
        : pajlada::Settings::Setting<Type>(path, defaultValue)
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

}  // namespace AB_NAMESPACE
