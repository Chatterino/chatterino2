#include "settings/settings.h"

#include <QDir>
#include <QStandardPaths>

namespace chatterino {
namespace settings {

StringSetting Settings::theme("", "dark");
StringSetting Settings::user("", "");
FloatSetting Settings::emoteScale("", 1.0);
BoolSetting Settings::scaleEmotesByLineHeight("", false);
BoolSetting Settings::showTimestamps("", true);
BoolSetting Settings::showTimestampSeconds("", false);
BoolSetting Settings::allowDouplicateMessages("", true);
BoolSetting Settings::linksDoubleClickOnly("", false);
BoolSetting Settings::hideEmptyInput("", false);
BoolSetting Settings::showMessageLength("", false);
BoolSetting Settings::seperateMessages("", false);
BoolSetting Settings::mentionUsersWithAt("", false);
BoolSetting Settings::allowCommandsAtEnd("", false);
BoolSetting Settings::enableHighlights("", true);
BoolSetting Settings::enableHighlightSound("", true);
BoolSetting Settings::enableHighlightTaskbar("", true);
BoolSetting Settings::customHighlightSound("", false);
BoolSetting Settings::enableTwitchEmotes("", true);
BoolSetting Settings::enableBttvEmotes("", true);
BoolSetting Settings::enableFFzEmotes("", true);
BoolSetting Settings::enableEmojis("", true);
BoolSetting Settings::enableGifAnimations("", true);
BoolSetting Settings::enableGifs("", true);
BoolSetting Settings::inlineWhispers("", true);
BoolSetting Settings::windowTopMost("", true);
BoolSetting Settings::compactTabs("", false);

QSettings Settings::settings(
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
    QSettings::IniFormat);

std::vector<Setting *> Settings::settingsItems;

bool Settings::portable(false);

messages::Word::Type Settings::wordTypeMask = messages::Word::Default;

int Settings::_ = Settings::_init();

void
Settings::save()
{
    for (Setting *item : settingsItems) {
        item->save(settings);
    }
}

void
Settings::load()
{
    for (Setting *item : settingsItems) {
        item->load(settings);
    }
}

bool
Settings::isIgnoredEmote(const QString &emote)
{
    return false;
}
}
}
