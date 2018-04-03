#pragma once

namespace chatterino {
namespace singletons {

static void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting);

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

    void saveRecall();

    ChatterinoSetting &operator=(const Type &newValue)
    {
        assert(this->data != nullptr);

        this->setValue(newValue);

        return *this;
    }

    template <typename T2>
    ChatterinoSetting &operator=(const T2 &newValue)
    {
        assert(this->data != nullptr);

        this->setValue(newValue);

        return *this;
    }

    ChatterinoSetting &operator=(Type &&newValue) noexcept
    {
        assert(this->data != nullptr);

        this->setValue(std::move(newValue));

        return *this;
    }

    bool operator==(const Type &rhs) const
    {
        assert(this->data != nullptr);

        return this->getValue() == rhs;
    }

    bool operator!=(const Type &rhs) const
    {
        assert(this->data != nullptr);

        return this->getValue() != rhs;
    }

    operator const Type() const
    {
        assert(this->data != nullptr);

        return this->getValue();
    }
};

}  // namespace singletons
}  // namespace chatterino
