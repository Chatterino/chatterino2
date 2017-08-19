#include "settingsmanager.hpp"
#include "appdatapath.hpp"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

using namespace chatterino::messages;

namespace chatterino {

SettingsManager::SettingsManager()
    : _settings(Path::getAppdataPath() + "settings.ini", QSettings::IniFormat)
    , showTimestamps("/appearance/messages/showTimestamps", true)
    , showTimestampSeconds("/appearance/messages/showTimestampSeconds", true)
    , showBadges("/appearance/messages/showBadges", true)
    , streamlinkPath("/behaviour/streamlinkPath", "")
    , emoteScale(_settingsItems, "emoteScale", 1.0)
    , mouseScrollMultiplier(_settingsItems, "mouseScrollMultiplier", 1.0)
    , scaleEmotesByLineHeight(_settingsItems, "scaleEmotesByLineHeight", false)
    , showLastMessageIndicator(_settingsItems, "showLastMessageIndicator", false)
    , allowDouplicateMessages(_settingsItems, "allowDouplicateMessages", true)
    , linksDoubleClickOnly(_settingsItems, "linksDoubleClickOnly", false)
    , hideEmptyInput(_settingsItems, "hideEmptyInput", false)
    , showMessageLength(_settingsItems, "showMessageLength", false)
    , seperateMessages(_settingsItems, "seperateMessages", false)
    , mentionUsersWithAt(_settingsItems, "mentionUsersWithAt", false)
    , allowCommandsAtEnd(_settingsItems, "allowCommandsAtEnd", false)
    , enableHighlights(_settingsItems, "enableHighlights", true)
    , enableHighlightsSelf(_settingsItems, "enableHighlightsSelf", true)
    , enableHighlightSound(_settingsItems, "enableHighlightSound", true)
    , enableHighlightTaskbar(_settingsItems, "enableHighlightTaskbar", true)
    , customHighlightSound(_settingsItems, "customHighlightSound", false)
    , pathHighlightSound(_settingsItems, "pathHighlightSound", "qrc:/sounds/ping2.wav")
    , highlightProperties(_settingsItems, "highlightProperties", QMap<QString, QPair<bool, bool>>())
    , enableTwitchEmotes(_settingsItems, "enableTwitchEmotes", true)
    , enableBttvEmotes(_settingsItems, "enableBttvEmotes", true)
    , enableFfzEmotes(_settingsItems, "enableFfzEmotes", true)
    , enableEmojis(_settingsItems, "enableEmojis", true)
    , enableGifAnimations(_settingsItems, "enableGifAnimations", true)
    , enableGifs(_settingsItems, "enableGifs", true)
    , inlineWhispers(_settingsItems, "inlineWhispers", true)
    , windowTopMost(_settingsItems, "windowTopMost", false)
    , hideTabX(_settingsItems, "hideTabX", false)
    , hidePreferencesButton(_settingsItems, "hidePreferencesButton", false)
    , hideUserButton(_settingsItems, "hideUserButton", false)
    , useCustomWindowFrame(_settingsItems, "useCustomWindowFrame", true)
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
    for (auto &item : _settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            _settings.setValue(item.get().getName(), item.get().getVariant());
        } else {
            _settings.beginGroup("Highlights");
            QStringList list = highlightProperties.get().keys();
            list.removeAll("");
            _settings.remove("");
            for (auto string : list) {
                _settings.beginGroup(string);
                _settings.setValue("highlightSound", highlightProperties.get().value(string).first);
                _settings.setValue("highlightTask", highlightProperties.get().value(string).second);
                _settings.endGroup();
            }
            _settings.endGroup();
        }
    }
}

void SettingsManager::load()
{
    for (auto &item : _settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            item.get().setVariant(_settings.value(item.get().getName()));
        } else {
            _settings.beginGroup("Highlights");
            QStringList list = _settings.childGroups();
            qDebug() << list.join(",");
            for (auto string : list) {
                _settings.beginGroup(string);
                highlightProperties.insertMap(string, _settings.value("highlightSound").toBool(),
                                              _settings.value("highlightTask").toBool());
                _settings.endGroup();
            }
            _settings.endGroup();
        }
    }
}

Word::Type SettingsManager::getWordTypeMask()
{
    return _wordTypeMask;
}

bool SettingsManager::isIgnoredEmote(const QString &)
{
    return false;
}

QSettings &SettingsManager::getQSettings()
{
    return _settings;
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

    Word::Type newMask = static_cast<Word::Type>(newMaskUint);

    if (newMask != _wordTypeMask) {
        _wordTypeMask = newMask;

        emit wordTypeMaskChanged();
    }
}

SettingsSnapshot SettingsManager::createSnapshot()
{
    SettingsSnapshot snapshot;

    for (auto &item : this->_settingsItems) {
        if (item.get().getName() != "highlightProperties") {
            snapshot.addItem(item, item.get().getVariant());
        } else {
            snapshot._mapItems = highlightProperties.get();
        }
    }

    return snapshot;
}

}  // namespace chatterino
