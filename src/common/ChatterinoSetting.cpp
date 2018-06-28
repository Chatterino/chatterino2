#include "common/ChatterinoSetting.hpp"

#include "singletons/Settings.hpp"

namespace chatterino {

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _actuallyRegisterSetting(setting);
}

}  // namespace chatterino
