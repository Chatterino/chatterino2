#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "messages/word.h"
#include "setting.h"
#include "settingssnapshot.h"

#include <QSettings>

namespace chatterino {

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings &
    getInstance()
    {
        return instance;
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

    QSettings &
    getQSettings()
    {
        return settings;
    }

    SettingsSnapshot
    createSnapshot()
    {
        SettingsSnapshot snapshot;

        for (auto &item : this->settingsItems) {
            snapshot.addItem(item, item.get().getVariant());
        }

        return snapshot;
    }

signals:
    void wordTypeMaskChanged();

private:
    Settings();

    static Settings instance;

    void updateWordTypeMask();

    QSettings settings;
    std::vector<std::reference_wrapper<BaseSetting>> settingsItems;

    bool portable;

    messages::Word::Type wordTypeMask;

public:
    Setting<QString> theme;
    Setting<QString> selectedUser;
    Setting<float> emoteScale;
    Setting<bool> scaleEmotesByLineHeight;
    Setting<bool> showTimestamps;
    Setting<bool> showTimestampSeconds;
    Setting<bool> showLastMessageIndicator;
    Setting<bool> allowDouplicateMessages;
    Setting<bool> linksDoubleClickOnly;
    Setting<bool> hideEmptyInput;
    Setting<bool> showMessageLength;
    Setting<bool> seperateMessages;
    Setting<bool> mentionUsersWithAt;
    Setting<bool> allowCommandsAtEnd;
    Setting<bool> enableHighlights;
    Setting<bool> enableHighlightSound;
    Setting<bool> enableHighlightTaskbar;
    Setting<bool> customHighlightSound;
    Setting<bool> enableTwitchEmotes;
    Setting<bool> enableBttvEmotes;
    Setting<bool> enableFfzEmotes;
    Setting<bool> enableEmojis;
    Setting<bool> enableGifAnimations;
    Setting<bool> enableGifs;
    Setting<bool> inlineWhispers;
    Setting<bool> windowTopMost;
    Setting<bool> hideTabX;
};

}  // namespace chatterino

#endif  // APPSETTINGS_H
