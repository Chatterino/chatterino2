#include "common/ChatterinoSetting.hpp"

#include "BaseSettings.hpp"

namespace AB_NAMESPACE {

void _registerSetting(std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _actuallyRegisterSetting(setting);
}

}  // namespace AB_NAMESPACE
