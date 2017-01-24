#include "settings.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

namespace chatterino {

Settings Settings::instance;

Settings::Settings()
    : settings(
          QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
              "/Chatterino/newsettings.ini",
          QSettings::IniFormat)
    , portable(false)
    , wordTypeMask(messages::Word::Default)
    , theme(this->settingsItems, "theme", "dark")
    , user(this->settingsItems, "selectedUser", "")
    , emoteScale(this->settingsItems, "emoteScale", 1.0)
    , scaleEmotesByLineHeight(this->settingsItems, "scaleEmotesByLineHeight",
                              false)
    , showTimestamps(this->settingsItems, "showTimestamps", true)
    , showTimestampSeconds(this->settingsItems, "showTimestampSeconds", false)
    , showLastMessageIndicator(this->settingsItems, "showLastMessageIndicator",
                               false)
    , allowDouplicateMessages(this->settingsItems, "allowDouplicateMessages",
                              true)
    , linksDoubleClickOnly(this->settingsItems, "linksDoubleClickOnly", false)
    , hideEmptyInput(this->settingsItems, "hideEmptyInput", false)
    , showMessageLength(this->settingsItems, "showMessageLength", false)
    , seperateMessages(this->settingsItems, "seperateMessages", false)
    , mentionUsersWithAt(this->settingsItems, "mentionUsersWithAt", false)
    , allowCommandsAtEnd(this->settingsItems, "allowCommandsAtEnd", false)
    , enableHighlights(this->settingsItems, "enableHighlights", true)
    , enableHighlightSound(this->settingsItems, "enableHighlightSound", true)
    , enableHighlightTaskbar(this->settingsItems, "enableHighlightTaskbar",
                             true)
    , customHighlightSound(this->settingsItems, "customHighlightSound", false)
    , enableTwitchEmotes(this->settingsItems, "enableTwitchEmotes", true)
    , enableBttvEmotes(this->settingsItems, "enableBttvEmotes", true)
    , enableFfzEmotes(this->settingsItems, "enableFfzEmotes", true)
    , enableEmojis(this->settingsItems, "enableEmojis", true)
    , enableGifAnimations(this->settingsItems, "enableGifAnimations", true)
    , enableGifs(this->settingsItems, "enableGifs", true)
    , inlineWhispers(this->settingsItems, "inlineWhispers", true)
    , windowTopMost(this->settingsItems, "windowTopMost", true)
    , hideTabX(this->settingsItems, "hideTabX", false)
{
    this->showTimestamps.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->showTimestampSeconds.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableBttvEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableEmojis.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableFfzEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
    this->enableTwitchEmotes.valueChanged.connect(
        [this](const auto &) { this->updateWordTypeMask(); });
}

void
Settings::save()
{
    for (auto &item : settingsItems) {
        item.get().save(settings);
    }
}

void
Settings::load()
{
    for (auto &item : settingsItems) {
        qDebug() << "Loading settings for " << item.get().getName();
        item.get().load(settings);
    }
}

bool
Settings::isIgnoredEmote(const QString &)
{
    return false;
}

void
Settings::updateWordTypeMask()

{
    using namespace messages;

    uint32_t mask = Word::Text;

    if (showTimestamps.get()) {
        mask |= showTimestampSeconds.get() ? Word::TimestampWithSeconds
                                           : Word::TimestampNoSeconds;
    }

    mask |= enableTwitchEmotes.get() ? Word::TwitchEmoteImage
                                     : Word::TwitchEmoteText;
    mask |= enableFfzEmotes.get() ? Word::FfzEmoteImage : Word::FfzEmoteText;
    mask |= enableBttvEmotes.get() ? Word::BttvEmoteImage : Word::BttvEmoteText;
    mask |= (enableBttvEmotes.get() && enableGifs.get()) ? Word::BttvEmoteImage
                                                         : Word::BttvEmoteText;
    mask |= enableEmojis.get() ? Word::EmojiImage : Word::EmojiText;

    mask |= Word::BitsAmount;
    mask |= enableGifs.get() ? Word::BitsAnimated : Word::BitsStatic;

    mask |= Word::Badges;
    mask |= Word::Username;

    Word::Type _mask = (Word::Type)mask;

    //    if (mask != _mask) {
    wordTypeMask = _mask;

    emit wordTypeMaskChanged();
    //    }
}

}  // namespace chatterino
