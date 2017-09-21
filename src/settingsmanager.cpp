#include "settingsmanager.hpp"
#include "appdatapath.hpp"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

using namespace chatterino::messages;

namespace chatterino {

SettingsManager::SettingsManager()
    : settings(Path::getAppdataPath() + "settings.ini", QSettings::IniFormat)
    , showTimestamps("/appearance/messages/showTimestamps", true)
    , showTimestampSeconds("/appearance/messages/showTimestampSeconds", true)
    , showBadges("/appearance/messages/showBadges", true)
    , streamlinkPath("/behaviour/streamlink/path", "")
    , preferredQuality("/behaviour/streamlink/quality", "Choose")
    , emoteScale(this->settingsItems, "emoteScale", 1.0)
    , mouseScrollMultiplier(this->settingsItems, "mouseScrollMultiplier", 1.0)
    , scaleEmotesByLineHeight(this->settingsItems, "scaleEmotesByLineHeight", false)
    , showLastMessageIndicator(this->settingsItems, "showLastMessageIndicator", false)
    , allowDouplicateMessages(this->settingsItems, "allowDouplicateMessages", true)
    , linksDoubleClickOnly(this->settingsItems, "linksDoubleClickOnly", false)
    , hideEmptyInput(this->settingsItems, "hideEmptyInput", false)
    , showMessageLength(this->settingsItems, "showMessageLength", false)
    , seperateMessages(this->settingsItems, "seperateMessages", false)
    , mentionUsersWithAt(this->settingsItems, "mentionUsersWithAt", false)
    , allowCommandsAtEnd(this->settingsItems, "allowCommandsAtEnd", false)
    , enableHighlights(this->settingsItems, "enableHighlights", true)
    , enableHighlightsSelf(this->settingsItems, "enableHighlightsSelf", true)
    , enableHighlightSound(this->settingsItems, "enableHighlightSound", true)
    , enableHighlightTaskbar(this->settingsItems, "enableHighlightTaskbar", true)
    , customHighlightSound(this->settingsItems, "customHighlightSound", false)
    , pathHighlightSound(this->settingsItems, "pathHighlightSound", "qrc:/sounds/ping2.wav")
    , highlightProperties(this->settingsItems, "highlightProperties",
                          QMap<QString, QPair<bool, bool>>())
    , enableTwitchEmotes(this->settingsItems, "enableTwitchEmotes", true)
    , enableBttvEmotes(this->settingsItems, "enableBttvEmotes", true)
    , enableFfzEmotes(this->settingsItems, "enableFfzEmotes", true)
    , enableEmojis(this->settingsItems, "enableEmojis", true)
    , enableGifAnimations(this->settingsItems, "enableGifAnimations", true)
    , enableGifs(this->settingsItems, "enableGifs", true)
    , inlineWhispers(this->settingsItems, "inlineWhispers", true)
    , windowTopMost(this->settingsItems, "windowTopMost", false)
    , hideTabX(this->settingsItems, "hideTabX", false)
    , hidePreferencesButton(this->settingsItems, "hidePreferencesButton", false)
    , hideUserButton(this->settingsItems, "hideUserButton", false)
    , useCustomWindowFrame(this->settingsItems, "useCustomWindowFrame", true)
{
    this->showTimestamps.getValueChangedSignal().connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->showTimestampSeconds.getValueChangedSignal().connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->showBadges.getValueChangedSignal().connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableBttvEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableEmojis.valueChanged.connect([this](const auto &) { this->updateWordTypeMask(); });
    this->enableFfzEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableTwitchEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
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

Word::Type SettingsManager::getWordTypeMask()
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

    newMaskUint |= enableTwitchEmotes.get() ? Word::TwitchEmoteImage : Word::TwitchEmoteText;
    newMaskUint |= enableFfzEmotes.get() ? Word::FfzEmoteImage : Word::FfzEmoteText;
    newMaskUint |= enableBttvEmotes.get() ? Word::BttvEmoteImage : Word::BttvEmoteText;
    newMaskUint |=
        (enableBttvEmotes.get() && enableGifs.get()) ? Word::BttvEmoteImage : Word::BttvEmoteText;
    newMaskUint |= enableEmojis.get() ? Word::EmojiImage : Word::EmojiText;

    newMaskUint |= Word::BitsAmount;
    newMaskUint |= enableGifs.get() ? Word::BitsAnimated : Word::BitsStatic;

    if (this->showBadges) {
        newMaskUint |= Word::Badges;
    }

    newMaskUint |= Word::Username;

    newMaskUint |= Word::AlwaysShow;

    Word::Type newMask = static_cast<Word::Type>(newMaskUint);

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
