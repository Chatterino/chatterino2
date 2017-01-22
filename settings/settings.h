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

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings &
    getInstance()
    {
        return _;
    }

    void load();
    void save();

    messages::Word::Type
    getWordTypeMask()
    {
        return wordTypeMask;
    }

    bool isIgnoredEmote(const QString &emote);

    bool
    getPortable()
    {
        return portable;
    }

    void
    setPortable(bool value)
    {
        portable = value;
    }

signals:
    void wordTypeMaskChanged();

private:
    Settings();

    static Settings _;

    void updateWordTypeMask(bool);

    QSettings settings;
    std::vector<Setting *> settingsItems;

    //    template <class T>
    //    T
    //    addSetting(T setting)
    //    {
    //        settingsItems.push_back(setting);
    //        return setting;
    //    }

    bool portable;

    messages::Word::Type wordTypeMask;

private:
    StringSetting theme;
    StringSetting user;
    FloatSetting emoteScale;
    BoolSetting scaleEmotesByLineHeight;
    BoolSetting showTimestamps;
    BoolSetting showTimestampSeconds;
    BoolSetting showLastMessageIndicator;
    BoolSetting allowDouplicateMessages;
    BoolSetting linksDoubleClickOnly;
    BoolSetting hideEmptyInput;
    BoolSetting showMessageLength;
    BoolSetting seperateMessages;
    BoolSetting mentionUsersWithAt;
    BoolSetting allowCommandsAtEnd;
    BoolSetting enableHighlights;
    BoolSetting enableHighlightSound;
    BoolSetting enableHighlightTaskbar;
    BoolSetting customHighlightSound;
    BoolSetting enableTwitchEmotes;
    BoolSetting enableBttvEmotes;
    BoolSetting enableFfzEmotes;
    BoolSetting enableEmojis;
    BoolSetting enableGifAnimations;
    BoolSetting enableGifs;
    BoolSetting inlineWhispers;
    BoolSetting windowTopMost;
    BoolSetting hideTabX;

    // settings
public:
    StringSetting &
    getTheme()
    {
        return this->theme;
    }
    StringSetting &
    getUser()
    {
        return this->user;
    }
    FloatSetting &
    getEmoteScale()
    {
        return this->emoteScale;
    }
    BoolSetting &
    getScaleEmotesByLineHeight()
    {
        return this->scaleEmotesByLineHeight;
    }
    BoolSetting &
    getShowTimestamps()
    {
        return this->showTimestamps;
    }
    BoolSetting &
    getShowTimestampSeconds()
    {
        return this->showTimestampSeconds;
    }
    BoolSetting &
    getShowLastMessageIndicator()
    {
        return this->showLastMessageIndicator;
    }
    BoolSetting &
    getAllowDouplicateMessages()
    {
        return this->allowDouplicateMessages;
    }
    BoolSetting &
    getLinksDoubleClickOnly()
    {
        return this->linksDoubleClickOnly;
    }
    BoolSetting &
    getHideEmptyInput()
    {
        return this->hideEmptyInput;
    }
    BoolSetting &
    getShowMessageLength()
    {
        return this->showMessageLength;
    }
    BoolSetting &
    getSeperateMessages()
    {
        return this->seperateMessages;
    }
    BoolSetting &
    getMentionUsersWithAt()
    {
        return this->mentionUsersWithAt;
    }
    BoolSetting &
    getAllowCommandsAtEnd()
    {
        return this->allowCommandsAtEnd;
    }
    BoolSetting &
    getEnableHighlights()
    {
        return this->enableHighlights;
    }
    BoolSetting &
    getEnableHighlightSound()
    {
        return this->enableHighlightSound;
    }
    BoolSetting &
    getEnableHighlightTaskbar()
    {
        return this->enableHighlightTaskbar;
    }
    BoolSetting &
    getCustomHighlightSound()
    {
        return this->customHighlightSound;
    }
    BoolSetting &
    getEnableTwitchEmotes()
    {
        return this->enableTwitchEmotes;
    }
    BoolSetting &
    getEnableBttvEmotes()
    {
        return this->enableBttvEmotes;
    }
    BoolSetting &
    getEnableFfzEmotes()
    {
        return this->enableFfzEmotes;
    }
    BoolSetting &
    getEnableEmojis()
    {
        return this->enableEmojis;
    }
    BoolSetting &
    getEnableGifAnimations()
    {
        return this->enableGifAnimations;
    }
    BoolSetting &
    getEnableGifs()
    {
        return this->enableGifs;
    }
    BoolSetting &
    getInlineWhispers()
    {
        return this->inlineWhispers;
    }
    BoolSetting &
    getWindowTopMost()
    {
        return this->windowTopMost;
    }
    BoolSetting &
    getHideTabX()
    {
        return this->hideTabX;
    }
};
}
}

#endif  // APPSETTINGS_H
