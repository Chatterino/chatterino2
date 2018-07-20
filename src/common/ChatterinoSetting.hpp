#pragma once

#include <QString>
#include <pajlada/settings.hpp>

namespace chatterino {

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting);

template <typename Type>
class ChatterinoSetting : public pajlada::Settings::Setting<Type>
{
public:
    ChatterinoSetting(const std::string &path)
        : pajlada::Settings::Setting<Type>(path)
    {
        _registerSetting(this->data);
    }

    ChatterinoSetting(const std::string &path, const Type &defaultValue)
        : pajlada::Settings::Setting<Type>(path, defaultValue)
    {
        _registerSetting(this->data);
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
using IntSetting = ChatterinoSetting<int>;
using StringSetting = ChatterinoSetting<std::string>;
using QStringSetting = ChatterinoSetting<QString>;

}  // namespace chatterino
