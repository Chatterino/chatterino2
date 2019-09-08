#include "BaseSettings.hpp"

#include <QDebug>

#include "util/Clamp.hpp"

namespace AB_NAMESPACE {

std::vector<std::weak_ptr<pajlada::Settings::SettingData>> _settings;

AB_SETTINGS_CLASS *AB_SETTINGS_CLASS::instance = nullptr;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _settings.push_back(setting);
}

AB_SETTINGS_CLASS::AB_SETTINGS_CLASS(const QString &settingsDirectory)
{
    AB_SETTINGS_CLASS::instance = this;

    QString settingsPath = settingsDirectory + "/settings.json";

    // get global instance of the settings library
    auto settingsInstance = pajlada::Settings::SettingManager::getInstance();

    settingsInstance->load(qPrintable(settingsPath));

    settingsInstance->setBackupEnabled(true);
    settingsInstance->setBackupSlots(9);
    settingsInstance->saveMethod =
        pajlada::Settings::SettingManager::SaveMethod::SaveOnExit;
}

void AB_SETTINGS_CLASS::saveSnapshot()
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

void AB_SETTINGS_CLASS::restoreSnapshot()
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

float AB_SETTINGS_CLASS::getClampedUiScale() const
{
    return clamp<float>(this->uiScale.getValue(), 0.2f, 10);
}

void AB_SETTINGS_CLASS::setClampedUiScale(float value)
{
    this->uiScale.setValue(clamp<float>(value, 0.2f, 10));
}

#ifndef AB_CUSTOM_SETTINGS
Settings *getSettings()
{
    static_assert(std::is_same_v<AB_SETTINGS_CLASS, Settings>,
                  "`AB_SETTINGS_CLASS` must be the same as `Settings`");

    assert(AB_SETTINGS_CLASS::instance);

    return AB_SETTINGS_CLASS::instance;
}
#endif

AB_SETTINGS_CLASS *getABSettings()
{
    assert(AB_SETTINGS_CLASS::instance);

    return AB_SETTINGS_CLASS::instance;
}

}  // namespace AB_NAMESPACE
