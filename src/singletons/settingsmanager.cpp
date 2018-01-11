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
    this->wordMaskListener.addSetting(this->showBadges);
    this->wordMaskListener.addSetting(this->enableBttvEmotes);
    this->wordMaskListener.addSetting(this->enableEmojis);
    this->wordMaskListener.addSetting(this->enableFfzEmotes);
    this->wordMaskListener.addSetting(this->enableTwitchEmotes);
    this->wordMaskListener.cb = [this](auto) {
        this->updateWordTypeMask();  //
    };
}

MessageElement::Flags SettingManager::getWordTypeMask()
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
    uint32_t newMaskUint = MessageElement::Text;

    if (this->showTimestamps) {
        newMaskUint |= MessageElement::Timestamp;
    }

    newMaskUint |=
        enableTwitchEmotes ? MessageElement::TwitchEmoteImage : MessageElement::TwitchEmoteText;
    newMaskUint |= enableFfzEmotes ? MessageElement::FfzEmoteImage : MessageElement::FfzEmoteText;
    newMaskUint |=
        enableBttvEmotes ? MessageElement::BttvEmoteImage : MessageElement::BttvEmoteText;
    newMaskUint |= enableEmojis ? MessageElement::EmojiImage : MessageElement::EmojiText;

    newMaskUint |= MessageElement::BitsAmount;
    newMaskUint |= enableGifAnimations ? MessageElement::BitsAnimated : MessageElement::BitsStatic;

    if (this->showBadges) {
        newMaskUint |= MessageElement::Badges;
    }

    newMaskUint |= MessageElement::Username;

    newMaskUint |= MessageElement::AlwaysShow;

    MessageElement::Flags newMask = static_cast<MessageElement::Flags>(newMaskUint);

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
