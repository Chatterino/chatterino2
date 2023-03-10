#pragma once

#include "common/ChatterinoSetting.hpp"

#include <pajlada/settings/settingdata.hpp>
#include <QString>
#include <rapidjson/document.h>

#include <memory>

#ifdef AB_CUSTOM_SETTINGS
#    define AB_SETTINGS_CLASS ABSettings
#else
#    define AB_SETTINGS_CLASS Settings
#endif

namespace chatterino {

class Settings;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting);

class AB_SETTINGS_CLASS
{
public:
    AB_SETTINGS_CLASS(const QString &settingsDirectory);

    void saveSnapshot();
    void restoreSnapshot();

    static AB_SETTINGS_CLASS *instance;

    FloatSetting uiScale = {"/appearance/uiScale2", 1};
    BoolSetting windowTopMost = {"/appearance/windowAlwaysOnTop", false};

    float getClampedUiScale() const;
    void setClampedUiScale(float value);

private:
    std::unique_ptr<rapidjson::Document> snapshot_;
};

Settings *getSettings();
AB_SETTINGS_CLASS *getABSettings();

}  // namespace chatterino
