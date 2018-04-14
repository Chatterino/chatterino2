#include "singletons/helper/chatterinosetting.hpp"

#include "singletons/settingsmanager.hpp"

namespace chatterino {
namespace singletons {

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _actuallyRegisterSetting(setting);
}

}  // namespace singletons
}  // namespace chatterino
