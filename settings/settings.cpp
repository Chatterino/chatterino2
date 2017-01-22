#include "settings/settings.h"

#include <QDir>
#include <QStandardPaths>

namespace chatterino {
namespace settings {

Settings Settings::instance;

Settings::Settings()
    : settings(
          QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
          QSettings::IniFormat)
    , portable(false)
    , wordTypeMask(messages::Word::Default)
    , theme("", "dark")
    , user("", "")
    , emoteScale("", 1.0)
    , scaleEmotesByLineHeight("", false)
    , showTimestamps("", true)
    , showTimestampSeconds("", false)
    , showLastMessageIndicator("", false)
    , allowDouplicateMessages("", true)
    , linksDoubleClickOnly("", false)
    , hideEmptyInput("", false)
    , showMessageLength("", false)
    , seperateMessages("", false)
    , mentionUsersWithAt("", false)
    , allowCommandsAtEnd("", false)
    , enableHighlights("", true)
    , enableHighlightSound("", true)
    , enableHighlightTaskbar("", true)
    , customHighlightSound("", false)
    , enableTwitchEmotes("", true)
    , enableBttvEmotes("", true)
    , enableFfzEmotes("", true)
    , enableEmojis("", true)
    , enableGifAnimations("", true)
    , enableGifs("", true)
    , inlineWhispers("", true)
    , windowTopMost("", true)
    , hideTabX("", false)
{
    settingsItems.push_back(theme);
    settingsItems.push_back(user);
    settingsItems.push_back(emoteScale);
    settingsItems.push_back(scaleEmotesByLineHeight);
    settingsItems.push_back(showTimestamps);
    settingsItems.push_back(showTimestampSeconds);
    settingsItems.push_back(showLastMessageIndicator);
    settingsItems.push_back(allowDouplicateMessages);
    settingsItems.push_back(linksDoubleClickOnly);
    settingsItems.push_back(hideEmptyInput);
    settingsItems.push_back(showMessageLength);
    settingsItems.push_back(seperateMessages);
    settingsItems.push_back(mentionUsersWithAt);
    settingsItems.push_back(allowCommandsAtEnd);
    settingsItems.push_back(enableHighlights);
    settingsItems.push_back(enableHighlightSound);
    settingsItems.push_back(enableHighlightTaskbar);
    settingsItems.push_back(customHighlightSound);
    settingsItems.push_back(enableTwitchEmotes);
    settingsItems.push_back(enableBttvEmotes);
    settingsItems.push_back(enableFfzEmotes);
    settingsItems.push_back(enableEmojis);
    settingsItems.push_back(enableGifAnimations);
    settingsItems.push_back(enableGifs);
    settingsItems.push_back(inlineWhispers);
    settingsItems.push_back(windowTopMost);
    settingsItems.push_back(hideTabX);

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

}  // namespace settings
}  // namespace chatterino
