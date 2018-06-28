#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino {

std::vector<std::weak_ptr<pajlada::Settings::ISettingData>> _settings;

void _actuallyRegisterSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _settings.push_back(setting);
}

Settings::Settings()
{
    qDebug() << "init SettingManager";
}

Settings &Settings::getInstance()
{
    static Settings instance;

    return instance;
}

void Settings::initialize()
{
    this->timestampFormat.connect([](auto, auto) {
        auto app = getApp();
        app->windows->layoutChannelViews();
    });

    this->emoteScale.connect([](auto, auto) { getApp()->windows->forceLayoutChannelViews(); });

    this->timestampFormat.connect([](auto, auto) { getApp()->windows->forceLayoutChannelViews(); });
    this->alternateMessageBackground.connect(
        [](auto, auto) { getApp()->windows->forceLayoutChannelViews(); });
    this->separateMessages.connect(
        [](auto, auto) { getApp()->windows->forceLayoutChannelViews(); });
    this->collpseMessagesMinLines.connect(
        [](auto, auto) { getApp()->windows->forceLayoutChannelViews(); });
}

void Settings::load()
{
    auto app = getApp();
    QString settingsPath = app->paths->settingsDirectory + "/settings.json";

    pajlada::Settings::SettingManager::load(qPrintable(settingsPath));
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

    this->snapshot.reset(d);

    Log("hehe: {}", pajlada::Settings::SettingManager::stringify(*d));
}

void Settings::restoreSnapshot()
{
    if (!this->snapshot) {
        return;
    }

    const auto &snapshotObject = this->snapshot->GetObject();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            Log("Error stage 1 of loading");
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshotObject.HasMember(path)) {
            Log("Error stage 2 of loading");
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
