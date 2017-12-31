#include "singletons/settingsmanager.hpp"
#include "appdatapath.hpp"
#include "debug/log.hpp"

#include <QDir>
#include <QStandardPaths>

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

std::vector<std::weak_ptr<pajlada::Settings::ISettingData>> _settings;

void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _settings.push_back(setting);
}

SettingManager::SettingManager()
    : streamlinkPath("/behaviour/streamlink/path", "")
    , preferredQuality("/behaviour/streamlink/quality", "Choose")
    , emoteScale(this->settingsItems, "emoteScale", 1.0)
    , pathHighlightSound(this->settingsItems, "pathHighlightSound", "qrc:/sounds/ping2.wav")
    , highlightProperties(this->settingsItems, "highlightProperties",
                          QMap<QString, QPair<bool, bool>>())
    , highlightUserBlacklist(this->settingsItems, "highlightUserBlacklist", "")
    , snapshot(nullptr)
    , settings(Path::getAppdataPath() + "settings.ini", QSettings::IniFormat)
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

void SettingManager::save()
{
    for (auto &item : this->settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            this->settings.setValue(item.get().getName(), item.get().getVariant());
        } else {
            this->settings.beginGroup("Highlights");
            QStringList list = highlightProperties.get().keys();
            list.removeAll("");
            this->settings.remove("");
            for (auto string : list) {
                this->settings.beginGroup(string);
                this->settings.setValue("highlightSound",
                                        highlightProperties.get().value(string).first);
                this->settings.setValue("highlightTask",
                                        highlightProperties.get().value(string).second);
                this->settings.endGroup();
            }
            this->settings.endGroup();
        }
    }
}

void SettingManager::load()
{
    for (auto &item : this->settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            item.get().setVariant(this->settings.value(item.get().getName()));
        } else {
            this->settings.beginGroup("Highlights");
            QStringList list = this->settings.childGroups();
            for (auto string : list) {
                this->settings.beginGroup(string);
                highlightProperties.insertMap(string,
                                              this->settings.value("highlightSound").toBool(),
                                              this->settings.value("highlightTask").toBool());
                this->settings.endGroup();
            }
            this->settings.endGroup();
        }
    }
}

Word::Flags SettingManager::getWordTypeMask()
{
    return this->wordTypeMask;
}

bool SettingManager::isIgnoredEmote(const QString &)
{
    return false;
}

QSettings &SettingManager::getQSettings()
{
    return this->settings;
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

}  // namespace chatterino
}
