#include "BaseSettings.hpp"

#include <QDebug>
#include <algorithm>

namespace chatterino
{
    std::vector<std::weak_ptr<pajlada::Settings::SettingData>> _settings;

    void _actuallyRegisterSetting(
        std::weak_ptr<pajlada::Settings::SettingData> setting)
    {
        _settings.push_back(setting);
    }

    BaseSettings::BaseSettings(const QString& settingsDirectory)
    {
        static bool uninitialized = true;
        assert(std::exchange(uninitialized, false));

        QString settingsPath = settingsDirectory + "/settings.json";

        // get global instance of the settings library
        auto settingsInstance =
            pajlada::Settings::SettingManager::getInstance();

        settingsInstance->load(qPrintable(settingsPath));

        settingsInstance->setBackupEnabled(true);
        settingsInstance->setBackupSlots(9);
        settingsInstance->saveMethod =
            pajlada::Settings::SettingManager::SaveMethod::SaveOnExit;
    }

    void BaseSettings::saveSnapshot()
    {
        rapidjson::Document* d =
            new rapidjson::Document(rapidjson::kObjectType);
        rapidjson::Document::AllocatorType& a = d->GetAllocator();

        for (const auto& weakSetting : _settings)
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

    void BaseSettings::restoreSnapshot()
    {
        if (!this->snapshot_)
        {
            return;
        }

        const auto& snapshot = *(this->snapshot_.get());

        if (!snapshot.IsObject())
        {
            return;
        }

        for (const auto& weakSetting : _settings)
        {
            auto setting = weakSetting.lock();
            if (!setting)
            {
                continue;
            }

            const char* path = setting->getPath().c_str();

            if (!snapshot.HasMember(path))
            {
                continue;
            }

            setting->marshalJSON(snapshot[path]);
        }
    }

}  // namespace chatterino
