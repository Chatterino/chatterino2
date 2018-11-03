#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino {

std::vector<std::weak_ptr<pajlada::Settings::SettingData>> _settings;

Settings *Settings::instance = nullptr;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _settings.push_back(setting);
}

Settings::Settings(Paths &paths)
{
    instance = this;

    QString settingsPath = paths.settingsDirectory + "/settings.json";

    // get global instance of the settings library
    auto settingsInstance = pajlada::Settings::SettingManager::getInstance();

    settingsInstance->load(qPrintable(settingsPath));

    settingsInstance->setBackupEnabled(true);
    settingsInstance->setBackupSlots(9);
    settingsInstance->saveMethod =
        pajlada::Settings::SettingManager::SaveMethod::SaveOnExit;
}

Settings &Settings::getInstance()
{
    return *instance;
}

void Settings::saveSnapshot()
{
    rapidjson::Document *d = new rapidjson::Document(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType &a = d->GetAllocator();

    for (const auto &weakSetting : _settings)
    {
        auto setting = weakSetting.lock();
        if (!setting)
        {
            continue;
        }

        rapidjson::Value key(setting->getPath().c_str(), a);
        auto curVal = setting->unmarshalJSON();
        if (curVal == nullptr)
        {
            continue;
        }

        rapidjson::Value val;
        val.CopyFrom(*curVal, a);
        d->AddMember(key.Move(), val.Move(), a);
    }

    // log("Snapshot state: {}", rj::stringify(*d));

    this->snapshot_.reset(d);
}

void Settings::restoreSnapshot()
{
    if (!this->snapshot_)
    {
        return;
    }

    const auto &snapshot = *(this->snapshot_.get());

    if (!snapshot.IsObject())
    {
        return;
    }

    for (const auto &weakSetting : _settings)
    {
        auto setting = weakSetting.lock();
        if (!setting)
        {
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshot.HasMember(path))
        {
            continue;
        }

        setting->marshalJSON(snapshot[path]);
    }
}

Settings *getSettings()
{
    return &Settings::getInstance();
}

}  // namespace chatterino
