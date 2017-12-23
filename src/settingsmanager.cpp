#include "settingsmanager.hpp"
#include "appdatapath.hpp"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

using namespace chatterino::messages;

namespace chatterino {

SettingsManager::SettingsManager()
    : streamlinkPath("/behaviour/streamlink/path", "")
    , preferredQuality("/behaviour/streamlink/quality", "Choose")
    , emoteScale(this->settingsItems, "emoteScale", 1.0)
    , pathHighlightSound(this->settingsItems, "pathHighlightSound", "qrc:/sounds/ping2.wav")
    , highlightProperties(this->settingsItems, "highlightProperties",
                          QMap<QString, QPair<bool, bool>>())
    , highlightUserBlacklist(this->settingsItems, "highlightUserBlacklist", "")
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

void SettingsManager::save()
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

void SettingsManager::load()
{
    for (auto &item : this->settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            item.get().setVariant(this->settings.value(item.get().getName()));
        } else {
            this->settings.beginGroup("Highlights");
            QStringList list = this->settings.childGroups();
            qDebug() << list.join(",");
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

Word::Flags SettingsManager::getWordTypeMask()
{
    return this->wordTypeMask;
}

bool SettingsManager::isIgnoredEmote(const QString &)
{
    return false;
}

QSettings &SettingsManager::getQSettings()
{
    return this->settings;
}

void SettingsManager::updateWordTypeMask()
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

SettingsSnapshot SettingsManager::createSnapshot()
{
    SettingsSnapshot snapshot;

    for (auto &item : this->settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            snapshot.addItem(item, item.get().getVariant());
        } else {
            snapshot.mapItems = highlightProperties.get();
        }
    }

    return snapshot;
}

}  // namespace chatterino
