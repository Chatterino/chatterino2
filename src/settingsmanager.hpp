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

    // new pajlada settings BBaper
    pajlada::Settings::Setting<bool> showTimestamps;
    pajlada::Settings::Setting<bool> showTimestampSeconds;
    pajlada::Settings::Setting<bool> showBadges;

    pajlada::Settings::Setting<std::string> streamlinkPath;
    pajlada::Settings::Setting<std::string> preferredQuality;

    // Settings
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
    Setting<QMap<QString, QPair<bool, bool>>> highlightProperties;
    Setting<QString> highlightUserBlacklist;
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

    static SettingsManager &getInstance()
    {
        static SettingsManager instance;
        return instance;
    }
    void updateWordTypeMask();

signals:
    void wordTypeMaskChanged();

private:
    SettingsManager();

    QSettings settings;
    std::vector<std::reference_wrapper<BaseSetting>> settingsItems;
    messages::Word::Type wordTypeMask = messages::Word::Default;
};

}  // namespace chatterino
