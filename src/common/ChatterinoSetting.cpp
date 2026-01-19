// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/ChatterinoSetting.hpp"

#include "singletons/Settings.hpp"

namespace chatterino {

void _registerSetting(std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _actuallyRegisterSetting(std::move(setting));
}

}  // namespace chatterino
