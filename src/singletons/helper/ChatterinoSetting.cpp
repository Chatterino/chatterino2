#include "singletons/helper/ChatterinoSetting.hpp"

#include "singletons/SettingsManager.hpp"

namespace chatterino {
namespace singletons {

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _actuallyRegisterSetting(setting);
}

}  // namespace singletons
}  // namespace chatterino
