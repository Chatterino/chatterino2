#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino {

std::vector<std::weak_ptr<pajlada::Settings::ISettingData>> _settings;

Settings *Settings::instance = nullptr;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _settings.push_back(setting);
}

Settings::Settings(Paths &paths)
{
    instance = this;

    QString settingsPath = paths.settingsDirectory + "/settings.json";

    pajlada::Settings::SettingManager::gLoad(qPrintable(settingsPath));
}

Settings &Settings::getInstance()
{
    return *instance;
}

void Settings::saveSnapshot()
{
    rapidjson::Document *d = new rapidjson::Document(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType &a = d->GetAllocator();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            continue;
        }

        rapidjson::Value key(setting->getPath().c_str(), a);
        rapidjson::Value val = setting->marshalInto(*d);
        d->AddMember(key.Move(), val.Move(), a);
    }

    this->snapshot_.reset(d);

    log("hehe: {}", pajlada::Settings::SettingManager::stringify(*d));
}

void Settings::restoreSnapshot()
{
    if (!this->snapshot_) {
        return;
    }

    const auto &snapshotObject = this->snapshot_->GetObject();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            log("Error stage 1 of loading");
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshotObject.HasMember(path)) {
            log("Error stage 2 of loading");
            continue;
        }

        setting->unmarshalValue(snapshotObject[path]);
    }
}

Settings *getSettings()
{
    return &Settings::getInstance();
}

}  // namespace chatterino
