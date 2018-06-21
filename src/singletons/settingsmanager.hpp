#pragma once

#include "controllers/highlights/highlightphrase.hpp"
#include "messages/messageelement.hpp"
#include "singletons/helper/chatterinosetting.hpp"
#include "singletons/helper/moderationaction.hpp"

#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>

namespace chatterino {
namespace singletons {

void _actuallyRegisterSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting);

class SettingManager
{
    using BoolSetting = ChatterinoSetting<bool>;
    using FloatSetting = ChatterinoSetting<float>;
    using IntSetting = ChatterinoSetting<int>;
    using StringSetting = ChatterinoSetting<std::string>;
    using QStringSetting = ChatterinoSetting<QString>;

public:
    SettingManager();

    ~SettingManager() = delete;

    messages::MessageElement::Flags getWordFlags();
    bool isIgnoredEmote(const QString &emote);

    void initialize();
    void load();

    /// Appearance
    BoolSetting showTimestamps = {"/appearance/messages/showTimestamps", true};
    QStringSetting timestampFormat = {"/appearance/messages/timestampFormat", "h:mm"};
    BoolSetting showBadges = {"/appearance/messages/showBadges", true};
    BoolSetting showLastMessageIndicator = {"/appearance/messages/showLastMessageIndicator", false};
    BoolSetting hideEmptyInput = {"/appearance/hideEmptyInputBox", false};
    BoolSetting showMessageLength = {"/appearance/messages/showMessageLength", false};
    BoolSetting seperateMessages = {"/appearance/messages/separateMessages", false};
    //    BoolSetting collapseLongMessages = {"/appearance/messages/collapseLongMessages", false};
    IntSetting collpseMessagesMinLines = {"/appearance/messages/collapseMessagesMinLines", 0};
    BoolSetting alternateMessageBackground = {"/appearance/messages/alternateMessageBackground",
                                              false};
    IntSetting uiScale = {"/appearance/uiScale", 0};
    BoolSetting windowTopMost = {"/appearance/windowAlwaysOnTop", false};
    BoolSetting showTabCloseButton = {"/appearance/showTabCloseButton", true};
    BoolSetting hidePreferencesButton = {"/appearance/hidePreferencesButton", false};
    BoolSetting hideUserButton = {"/appearance/hideUserButton", false};
    BoolSetting enableSmoothScrolling = {"/appearance/smoothScrolling", true};
    BoolSetting enableSmoothScrollingNewMessages = {"/appearance/smoothScrollingNewMessages",
                                                    false};
    // BoolSetting useCustomWindowFrame = {"/appearance/useCustomWindowFrame", false};

    /// Behaviour
    BoolSetting allowDuplicateMessages = {"/behaviour/allowDuplicateMessages", true};
    BoolSetting mentionUsersWithAt = {"/behaviour/mentionUsersWithAt", false};
    BoolSetting showJoins = {"/behaviour/showJoins", false};
    BoolSetting showParts = {"/behaviour/showParts", false};
    FloatSetting mouseScrollMultiplier = {"/behaviour/mouseScrollMultiplier", 1.0};

    // Auto-completion
    BoolSetting onlyFetchChattersForSmallerStreamers = {
        "/behaviour/autocompletion/onlyFetchChattersForSmallerStreamers", true};
    IntSetting smallStreamerLimit = {"/behaviour/autocompletion/smallStreamerLimit", 1000};

    BoolSetting pauseChatHover = {"/behaviour/pauseChatHover", false};

    /// Commands
    BoolSetting allowCommandsAtEnd = {"/commands/allowCommandsAtEnd", false};

    /// Emotes
    BoolSetting scaleEmotesByLineHeight = {"/emotes/scaleEmotesByLineHeight", false};
    BoolSetting enableTwitchEmotes = {"/emotes/enableTwitchEmotes", true};
    BoolSetting enableBttvEmotes = {"/emotes/enableBTTVEmotes", true};
    BoolSetting enableFfzEmotes = {"/emotes/enableFFZEmotes", true};
    BoolSetting enableEmojis = {"/emotes/enableEmojis", true};
    BoolSetting enableGifAnimations = {"/emotes/enableGifAnimations", true};
    FloatSetting emoteScale = {"/emotes/scale", 1.f};

    // 0 = No preference
    // 1 = 1x
    // 2 = 2x
    // 3 = 3x
    IntSetting preferredEmoteQuality = {"/emotes/preferredEmoteQuality", 0};

    QStringSetting emojiSet = {"/emotes/emojiSet", "EmojiOne 2"};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};

    /// Ingored Users
    BoolSetting enableTwitchIgnoredUsers = {"/ignore/enableTwitchIgnoredUsers", true};

    /// Moderation
    QStringSetting moderationActions = {"/moderation/actions", "/ban {user}\n/timeout {user} 300"};
    QStringSetting timeoutAction = {"/moderation/timeoutAction", "Disable"};

    /// Highlighting
    //    BoolSetting enableHighlights = {"/highlighting/enabled", true};
    BoolSetting enableHighlightsSelf = {"/highlighting/nameIsHighlightKeyword", true};
    BoolSetting enableHighlightSound = {"/highlighting/enableSound", true};
    BoolSetting enableHighlightTaskbar = {"/highlighting/enableTaskbarFlashing", true};
    BoolSetting customHighlightSound = {"/highlighting/useCustomSound", false};

    /// Logging
    BoolSetting enableLogging = {"/logging/enabled", false};

    QStringSetting logPath = {"/logging/path", ""};

    QStringSetting pathHighlightSound = {"/highlighting/highlightSoundPath",
                                         "qrc:/sounds/ping2.wav"};
    QStringSetting highlightUserBlacklist = {"/highlighting/blacklistedUsers", ""};

    BoolSetting highlightAlwaysPlaySound = {"/highlighting/alwaysPlaySound", false};

    BoolSetting inlineWhispers = {"/whispers/enableInlineWhispers", true};

    /// External tools
    // Streamlink
    BoolSetting streamlinkUseCustomPath = {"/external/streamlink/useCustomPath", false};
    QStringSetting streamlinkPath = {"/external/streamlink/customPath", ""};
    QStringSetting preferredQuality = {"/external/streamlink/quality", "Choose"};
    QStringSetting streamlinkOpts = {"/external/streamlink/options", ""};

    /// Misc
    IntSetting startUpNotification = {"/misc/startUpNotification", 0};

    void updateWordTypeMask();

    void saveSnapshot();
    void recallSnapshot();

    std::vector<ModerationAction> getModerationActions() const;
    pajlada::Signals::NoArgSignal wordFlagsChanged;

private:
    std::vector<ModerationAction> _moderationActions;
    std::unique_ptr<rapidjson::Document> snapshot;

    void updateModerationActions();

    messages::MessageElement::Flags wordFlags = messages::MessageElement::Default;

    pajlada::Settings::SettingListener wordFlagsListener;
};

}  // namespace singletons
}  // namespace chatterino
