#pragma once

#include "messages/word.hpp"
#include "setting.hpp"

#include <QSettings>
#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>

namespace chatterino {

static void _registerSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting);

template <typename Type>
class ChatterinoSetting : public pajlada::Settings::Setting<Type>
{
public:
    ChatterinoSetting(const std::string &_path, const Type &_defaultValue)
        : pajlada::Settings::Setting<Type>(_path, _defaultValue)
    {
        _registerSetting(this->data);
    }

    void saveRecall();

    ChatterinoSetting &operator=(const Type &newValue)
    {
        assert(this->data != nullptr);

        this->setValue(newValue);

        return *this;
    }

    template <typename T2>
    ChatterinoSetting &operator=(const T2 &newValue)
    {
        assert(this->data != nullptr);

        this->setValue(newValue);

        return *this;
    }

    ChatterinoSetting &operator=(Type &&newValue) noexcept
    {
        assert(this->data != nullptr);

        this->setValue(std::move(newValue));

        return *this;
    }

    bool operator==(const Type &rhs) const
    {
        assert(this->data != nullptr);

        return this->getValue() == rhs;
    }

    bool operator!=(const Type &rhs) const
    {
        assert(this->data != nullptr);

        return this->getValue() != rhs;
    }

    operator const Type() const
    {
        assert(this->data != nullptr);

        return this->getValue();
    }
};

class SettingsManager : public QObject
{
    Q_OBJECT

    using BoolSetting = ChatterinoSetting<bool>;
    using FloatSetting = ChatterinoSetting<float>;

public:
    void load();
    void save();

    messages::Word::Flags getWordTypeMask();
    bool isIgnoredEmote(const QString &emote);
    QSettings &getQSettings();

    /// Appearance
    BoolSetting showTimestamps = {"/appearance/messages/showTimestamps", true};
    BoolSetting showTimestampSeconds = {"/appearance/messages/showTimestampSeconds", true};
    BoolSetting showBadges = {"/appearance/messages/showBadges", true};
    BoolSetting showLastMessageIndicator = {"/appearance/messages/showLastMessageIndicator", false};
    BoolSetting hideEmptyInput = {"/appearance/hideEmptyInputBox", false};
    BoolSetting showMessageLength = {"/appearance/messages/showMessageLength", false};
    BoolSetting seperateMessages = {"/appearance/messages/separateMessages", false};
    BoolSetting windowTopMost = {"/appearance/windowAlwaysOnTop", false};
    BoolSetting hideTabX = {"/appearance/hideTabX", false};
    BoolSetting hidePreferencesButton = {"/appearance/hidePreferencesButton", false};
    BoolSetting hideUserButton = {"/appearance/hideUserButton", false};
    BoolSetting enableSmoothScrolling = {"/appearance/smoothScrolling", true};
    // BoolSetting useCustomWindowFrame = {"/appearance/useCustomWindowFrame", false};

    /// Behaviour
    BoolSetting allowDuplicateMessages = {"/behaviour/allowDuplicateMessages", true};
    BoolSetting mentionUsersWithAt = {"/behaviour/mentionUsersWithAt", false};
    FloatSetting mouseScrollMultiplier = {"/behaviour/mouseScrollMultiplier", 1.0};

    /// Commands
    BoolSetting allowCommandsAtEnd = {"/commands/allowCommandsAtEnd", false};

    /// Emotes
    BoolSetting scaleEmotesByLineHeight = {"/emotes/scaleEmotesByLineHeight", false};
    BoolSetting enableTwitchEmotes = {"/emotes/enableTwitchEmotes", true};
    BoolSetting enableBttvEmotes = {"/emotes/enableBTTVEmotes", true};
    BoolSetting enableFfzEmotes = {"/emotes/enableFFZEmotes", true};
    BoolSetting enableEmojis = {"/emotes/enableEmojis", true};
    BoolSetting enableGifAnimations = {"/emotes/enableGifAnimations", true};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};

    /// Highlighting
    BoolSetting enableHighlights = {"/highlighting/enabled", true};
    BoolSetting enableHighlightsSelf = {"/highlighting/nameIsHighlightKeyword", true};
    BoolSetting enableHighlightSound = {"/highlighting/enableSound", true};
    BoolSetting enableHighlightTaskbar = {"/highlighting/enableTaskbarFlashing", true};
    BoolSetting customHighlightSound = {"/highlighting/useCustomSound", false};

    pajlada::Settings::Setting<std::string> streamlinkPath;
    pajlada::Settings::Setting<std::string> preferredQuality;

    Setting<float> emoteScale;

    Setting<QString> pathHighlightSound;
    Setting<QMap<QString, QPair<bool, bool>>> highlightProperties;
    Setting<QString> highlightUserBlacklist;

    BoolSetting highlightAlwaysPlaySound = {"/highlighting/alwaysPlaySound", false};

    BoolSetting inlineWhispers = {"/whispers/enableInlineWhispers", true};

    static SettingsManager &getInstance()
    {
        static SettingsManager instance;
        return instance;
    }
    void updateWordTypeMask();

    void saveSnapshot();
    void recallSnapshot();

signals:
    void wordTypeMaskChanged();

private:
    std::unique_ptr<rapidjson::Document> snapshot;

    SettingsManager();

    QSettings settings;
    std::vector<std::reference_wrapper<BaseSetting>> settingsItems;
    messages::Word::Flags wordTypeMask = messages::Word::Default;

    pajlada::Settings::SettingListener wordMaskListener;
};

}  // namespace chatterino
