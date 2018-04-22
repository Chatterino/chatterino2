#pragma once

namespace chatterino {
namespace singletons {

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting);

template <typename Type>
class ChatterinoSetting : public pajlada::Settings::Setting<Type>
{
public:
    ChatterinoSetting(const std::string &_path)
        : pajlada::Settings::Setting<Type>(_path)
    {
        _registerSetting(this->data);
    }

    ChatterinoSetting(const std::string &_path, const Type &_defaultValue)
        : pajlada::Settings::Setting<Type>(_path, _defaultValue)
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

    using pajlada::Settings::Setting<Type>::operator const Type;
};

}  // namespace singletons
}  // namespace chatterino
