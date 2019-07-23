#ifndef AB_SETTINGS_H
#define AB_SETTINGS_H

#include <rapidjson/document.h>
#include <QString>
#include <memory>
#include <pajlada/settings/settingdata.hpp>

#include "common/ChatterinoSetting.hpp"

#ifdef AB_CUSTOM_SETTINGS
#    define AB_SETTINGS_CLASS ABSettings
#else
#    define AB_SETTINGS_CLASS Settings
#endif

namespace AB_NAMESPACE {

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

}  // namespace AB_NAMESPACE

#ifdef CHATTERINO
#    include "singletons/Settings.hpp"
#endif
#endif
