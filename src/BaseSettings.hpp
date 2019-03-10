#pragma once

#include <rapidjson/document.h>
#include <QString>
#include <memory>
#include <pajlada/settings/settingdata.hpp>

#include "util/ChatterinoSetting.hpp"

namespace chatterino
{
    void _actuallyRegisterSetting(
        std::weak_ptr<pajlada::Settings::SettingData> setting);

    class BaseSettings
    {
    public:
        BaseSettings(const QString& settingsDirectory);

        void saveSnapshot();
        void restoreSnapshot();

    private:
        std::unique_ptr<rapidjson::Document> snapshot_;
    };
}  // namespace chatterino
