#pragma once

#include "messages/word.hpp"
#include "setting.hpp"
#include "settingssnapshot.hpp"

#include <QSettings>
#include <pajlada/settings/setting.hpp>

namespace chatterino {

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    void load();
    void save();

    messages::Word::Type getWordTypeMask();
    bool isIgnoredEmote(const QString &emote);
    QSettings &getQSettings();
    SettingsSnapshot createSnapshot();

signals:
    void wordTypeMaskChanged();

private:
    SettingsManager();

    // variables
    QSettings _settings;
    std::vector<std::reference_wrapper<BaseSetting>> _settingsItems;
    messages::Word::Type _wordTypeMask = messages::Word::Default;

    // methods
public: // temporary
    void updateWordTypeMask();

public:
    // new pajlada settings BBaper
    pajlada::Settings::Setting<bool> showTimestamps;
    pajlada::Settings::Setting<bool> showTimestampSeconds;
    pajlada::Settings::Setting<bool> showBadges;

    // Settings
    Setting<QString> selectedUser;
    Setting<float> emoteScale;
    Setting<float> mouseScrollMultiplier;
    Setting<bool> scaleEmotesByLineHeight;
    Setting<bool> showLastMessageIndicator;
    Setting<bool> allowDouplicateMessages;
    Setting<bool> linksDoubleClickOnly;
    Setting<bool> hideEmptyInput;
    Setting<bool> showMessageLength;
    Setting<bool> seperateMessages;
    Setting<bool> mentionUsersWithAt;
    Setting<bool> allowCommandsAtEnd;
    Setting<bool> enableHighlights;
    Setting<bool> enableHighlightsSelf;
    Setting<bool> enableHighlightSound;
    Setting<bool> enableHighlightTaskbar;
    Setting<bool> customHighlightSound;
    Setting<QString> pathHighlightSound;
    Setting<QMap<QString,QPair<bool,bool>>> highlightProperties;
    Setting<bool> enableTwitchEmotes;
    Setting<bool> enableBttvEmotes;
    Setting<bool> enableFfzEmotes;
    Setting<bool> enableEmojis;
    Setting<bool> enableGifAnimations;
    Setting<bool> enableGifs;
    Setting<bool> inlineWhispers;
    Setting<bool> windowTopMost;
    Setting<bool> hideTabX;
    Setting<bool> hidePreferencesButton;
    Setting<bool> hideUserButton;
    Setting<bool> useCustomWindowFrame;

public:
    static SettingsManager &getInstance()
    {
        static SettingsManager instance;
        return instance;
    }
};

}  // namespace chatterino
