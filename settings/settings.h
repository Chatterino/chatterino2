#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "messages/word.h"
#include "settings/boolsetting.h"
#include "settings/floatsetting.h"
#include "settings/setting.h"
#include "settings/stringsetting.h"

#include <QSettings>

namespace chatterino {
namespace settings {

class Settings
{
public:
    static messages::Word::Type
    getWordTypeMask()
    {
        return wordTypeMask;
    }

    static bool isIgnoredEmote(const QString &emote);

    static void load();
    static void save();

    static bool
    getPortable()
    {
        return portable;
    }

    static void
    setPortable(bool value)
    {
        portable = value;
    }

private:
    Settings();

    static int _;
    static int
    _init()
    {
        settingsItems.reserve(25);
        settingsItems.push_back(&theme);
        settingsItems.push_back(&user);
        settingsItems.push_back(&emoteScale);
        settingsItems.push_back(&scaleEmotesByLineHeight);
        settingsItems.push_back(&showTimestamps);
        settingsItems.push_back(&showTimestampSeconds);
        settingsItems.push_back(&allowDouplicateMessages);
        settingsItems.push_back(&linksDoubleClickOnly);
        settingsItems.push_back(&hideEmptyInput);
        settingsItems.push_back(&showMessageLength);
        settingsItems.push_back(&seperateMessages);
        settingsItems.push_back(&mentionUsersWithAt);
        settingsItems.push_back(&allowCommandsAtEnd);
        settingsItems.push_back(&enableHighlights);
        settingsItems.push_back(&enableHighlightSound);
        settingsItems.push_back(&enableHighlightTaskbar);
        settingsItems.push_back(&customHighlightSound);
        settingsItems.push_back(&enableTwitchEmotes);
        settingsItems.push_back(&enableBttvEmotes);
        settingsItems.push_back(&enableFFzEmotes);
        settingsItems.push_back(&enableEmojis);
        settingsItems.push_back(&enableGifAnimations);
        settingsItems.push_back(&enableGifs);
        settingsItems.push_back(&inlineWhispers);
        settingsItems.push_back(&windowTopMost);
        settingsItems.push_back(&compactTabs);
    }

    static QSettings settings;
    static std::vector<Setting *> settingsItems;

    template <class T>
    static T
    addSetting(T setting)
    {
        settingsItems.push_back(setting);
        return setting;
    }

    static bool portable;

    static messages::Word::Type wordTypeMask;

    // settings
public:
    static StringSetting
    getTheme()
    {
        return Settings::theme;
    }
    static StringSetting
    getUser()
    {
        return Settings::user;
    }
    static FloatSetting
    getEmoteScale()
    {
        return Settings::emoteScale;
    }
    static BoolSetting
    getScaleEmotesByLineHeight()
    {
        return Settings::scaleEmotesByLineHeight;
    }
    static BoolSetting
    getShowTimestamps()
    {
        return Settings::showTimestamps;
    }
    static BoolSetting
    getShowTimestampSeconds()
    {
        return Settings::showTimestampSeconds;
    }
    static BoolSetting
    getAllowDouplicateMessages()
    {
        return Settings::allowDouplicateMessages;
    }
    static BoolSetting
    getLinksDoubleClickOnly()
    {
        return Settings::linksDoubleClickOnly;
    }
    static BoolSetting
    getHideEmptyInput()
    {
        return Settings::hideEmptyInput;
    }
    static BoolSetting
    getShowMessageLength()
    {
        return Settings::showMessageLength;
    }
    static BoolSetting
    getSeperateMessages()
    {
        return Settings::seperateMessages;
    }
    static BoolSetting
    getMentionUsersWithAt()
    {
        return Settings::mentionUsersWithAt;
    }
    static BoolSetting
    getAllowCommandsAtEnd()
    {
        return Settings::allowCommandsAtEnd;
    }
    static BoolSetting
    getEnableHighlights()
    {
        return Settings::enableHighlights;
    }
    static BoolSetting
    getEnableHighlightSound()
    {
        return Settings::enableHighlightSound;
    }
    static BoolSetting
    getEnableHighlightTaskbar()
    {
        return Settings::enableHighlightTaskbar;
    }
    static BoolSetting
    getCustomHighlightSound()
    {
        return Settings::customHighlightSound;
    }
    static BoolSetting
    getEnableTwitchEmotes()
    {
        return Settings::enableTwitchEmotes;
    }
    static BoolSetting
    getEnableBttvEmotes()
    {
        return Settings::enableBttvEmotes;
    }
    static BoolSetting
    getEnableFFzEmotes()
    {
        return Settings::enableFFzEmotes;
    }
    static BoolSetting
    getEnableEmojis()
    {
        return Settings::enableEmojis;
    }
    static BoolSetting
    getEnableGifAnimations()
    {
        return Settings::enableGifAnimations;
    }
    static BoolSetting
    getEnableGifs()
    {
        return Settings::enableGifs;
    }
    static BoolSetting
    getInlineWhispers()
    {
        return Settings::inlineWhispers;
    }
    static BoolSetting
    getWindowTopMost()
    {
        return Settings::windowTopMost;
    }
    static BoolSetting
    getCompactTabs()
    {
        return Settings::compactTabs;
    }

private:
    static StringSetting theme;
    static StringSetting user;
    static FloatSetting emoteScale;
    static BoolSetting scaleEmotesByLineHeight;
    static BoolSetting showTimestamps;
    static BoolSetting showTimestampSeconds;
    static BoolSetting allowDouplicateMessages;
    static BoolSetting linksDoubleClickOnly;
    static BoolSetting hideEmptyInput;
    static BoolSetting showMessageLength;
    static BoolSetting seperateMessages;
    static BoolSetting mentionUsersWithAt;
    static BoolSetting allowCommandsAtEnd;
    static BoolSetting enableHighlights;
    static BoolSetting enableHighlightSound;
    static BoolSetting enableHighlightTaskbar;
    static BoolSetting customHighlightSound;
    static BoolSetting enableTwitchEmotes;
    static BoolSetting enableBttvEmotes;
    static BoolSetting enableFFzEmotes;
    static BoolSetting enableEmojis;
    static BoolSetting enableGifAnimations;
    static BoolSetting enableGifs;
    static BoolSetting inlineWhispers;
    static BoolSetting windowTopMost;
    static BoolSetting compactTabs;
};
}
}

#endif  // APPSETTINGS_H
