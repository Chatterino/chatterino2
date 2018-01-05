#include "singletons/settingsmanager.hpp"
#include "debug/log.hpp"
#include "singletons/pathmanager.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

std::vector<std::weak_ptr<pajlada::Settings::ISettingData>> _settings;

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _settings.push_back(setting);
}

SettingManager::SettingManager()
    : snapshot(nullptr)
{
    this->wordMaskListener.addSetting(this->showTimestamps);
    this->wordMaskListener.addSetting(this->showTimestampSeconds);
    this->wordMaskListener.addSetting(this->showBadges);
    this->wordMaskListener.addSetting(this->enableBttvEmotes);
    this->wordMaskListener.addSetting(this->enableEmojis);
    this->wordMaskListener.addSetting(this->enableFfzEmotes);
    this->wordMaskListener.addSetting(this->enableTwitchEmotes);
    this->wordMaskListener.cb = [this](auto) {
        this->updateWordTypeMask();  //
    };
}

Word::Flags SettingManager::getWordTypeMask()
{
    return this->wordTypeMask;
}

bool SettingManager::isIgnoredEmote(const QString &)
{
    return false;
}

void SettingManager::init()
{
    QString settingsPath = PathManager::getInstance().settingsFolderPath + "/settings.json";

    pajlada::Settings::SettingManager::load(qPrintable(settingsPath));
}

void SettingManager::updateWordTypeMask()
{
    uint32_t newMaskUint = Word::Text;

    if (this->showTimestamps) {
        if (this->showTimestampSeconds) {
            newMaskUint |= Word::TimestampWithSeconds;
        } else {
            newMaskUint |= Word::TimestampNoSeconds;
        }
    }

    newMaskUint |= enableTwitchEmotes ? Word::TwitchEmoteImage : Word::TwitchEmoteText;
    newMaskUint |= enableFfzEmotes ? Word::FfzEmoteImage : Word::FfzEmoteText;
    newMaskUint |= enableBttvEmotes ? Word::BttvEmoteImage : Word::BttvEmoteText;
    newMaskUint |=
        (enableBttvEmotes && enableGifAnimations) ? Word::BttvEmoteImage : Word::BttvEmoteText;
    newMaskUint |= enableEmojis ? Word::EmojiImage : Word::EmojiText;

    newMaskUint |= Word::BitsAmount;
    newMaskUint |= enableGifAnimations ? Word::BitsAnimated : Word::BitsStatic;

    if (this->showBadges) {
        newMaskUint |= Word::Badges;
    }

    newMaskUint |= Word::Username;

    newMaskUint |= Word::AlwaysShow;

    Word::Flags newMask = static_cast<Word::Flags>(newMaskUint);

    if (newMask != this->wordTypeMask) {
        this->wordTypeMask = newMask;

        emit wordTypeMaskChanged();
    }
}

void SettingManager::saveSnapshot()
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

    debug::Log("hehe: {}", pajlada::Settings::SettingManager::stringify(*d));
}

void SettingManager::recallSnapshot()
{
    if (!this->snapshot) {
        return;
    }

    const auto &snapshotObject = this->snapshot->GetObject();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            debug::Log("Error stage 1 of loading");
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshotObject.HasMember(path)) {
            debug::Log("Error stage 2 of loading");
            continue;
        }

        setting->unmarshalValue(snapshotObject[path]);
    }
}

}  // namespace singletons
}  // namespace chatterino
