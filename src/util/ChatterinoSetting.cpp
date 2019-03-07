#include "common/ChatterinoSetting.hpp"

#include "BaseSettings.hpp"

namespace chatterino
{
    void _registerSetting(std::weak_ptr<pajlada::Settings::SettingData> setting)
    {
        _actuallyRegisterSetting(setting);
    }
}  // namespace chatterino
