#pragma once

#include "Paths.hpp"

#include "common/ChatterinoSetting.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"

#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>

namespace chatterino {

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::ISettingData> setting);

class Settings
{
    static Settings *instance;

public:
    Settings(Paths &paths);

    static Settings &getInstance();

    /// Appearance
    BoolSetting showTimestamps = {"/appearance/messages/showTimestamps", true};
    BoolSetting enableAnimationsWhenFocused = {
        "/appearance/enableAnimationsWhenFocused", false};
    QStringSetting timestampFormat = {"/appearance/messages/timestampFormat",
                                      "h:mm"};
    BoolSetting showLastMessageIndicator = {
        "/appearance/messages/showLastMessageIndicator", false};
    IntSetting lastMessagePattern = {"/appearance/messages/lastMessagePattern",
                                     Qt::VerPattern};
    QStringSetting lastMessageColor = {"/appearance/messages/lastMessageColor",
                                       ""};
    BoolSetting showEmptyInput = {"/appearance/showEmptyInputBox", true};
    BoolSetting showMessageLength = {"/appearance/messages/showMessageLength",
                                     false};
    BoolSetting separateMessages = {"/appearance/messages/separateMessages",
                                    false};
    //    BoolSetting collapseLongMessages =
    //    {"/appearance/messages/collapseLongMessages", false};
    IntSetting collpseMessagesMinLines = {
        "/appearance/messages/collapseMessagesMinLines", 0};
    BoolSetting alternateMessageBackground = {
        "/appearance/messages/alternateMessageBackground", false};
    IntSetting uiScale = {"/appearance/uiScale", 0};
    IntSetting boldScale = {"/appearance/boldScale", 57};
    BoolSetting windowTopMost = {"/appearance/windowAlwaysOnTop", false};
    BoolSetting showTabCloseButton = {"/appearance/showTabCloseButton", true};
    BoolSetting hidePreferencesButton = {"/appearance/hidePreferencesButton",
                                         false};
    BoolSetting hideUserButton = {"/appearance/hideUserButton", false};
    BoolSetting enableSmoothScrolling = {"/appearance/smoothScrolling", true};
    BoolSetting enableSmoothScrollingNewMessages = {
        "/appearance/smoothScrollingNewMessages", false};
    BoolSetting enableUsernameBold = {"/appearance/messages/boldUsernames",
                                      false};
    // BoolSetting customizable splitheader
    BoolSetting showViewerCount = {"/appearance/splitheader/showViewerCount",
                                   false};
    BoolSetting showTitle = {"/appearance/splitheader/showTitle", false};
    BoolSetting showGame = {"/appearance/splitheader/showGame", false};
    BoolSetting showUptime = {"/appearance/splitheader/showUptime", false};
    FloatSetting customThemeMultiplier = {"/appearance/customThemeMultiplier",
                                          -0.5f};
    // BoolSetting useCustomWindowFrame = {"/appearance/useCustomWindowFrame",
    // false};

    // Badges
    BoolSetting showBadgesGlobalAuthority = {
        "/appearance/badges/GlobalAuthority", true};
    BoolSetting showBadgesChannelAuthority = {
        "/appearance/badges/ChannelAuthority", true};
    BoolSetting showBadgesSubscription = {"/appearance/badges/subscription",
                                          true};
    BoolSetting showBadgesVanity = {"/appearance/badges/vanity", true};
    BoolSetting showBadgesChatterino = {"/appearance/badges/chatterino", true};

    /// Behaviour
    BoolSetting allowDuplicateMessages = {"/behaviour/allowDuplicateMessages",
                                          true};
    BoolSetting mentionUsersWithAt = {"/behaviour/mentionUsersWithAt", false};
    BoolSetting showJoins = {"/behaviour/showJoins", false};
    BoolSetting showParts = {"/behaviour/showParts", false};
    FloatSetting mouseScrollMultiplier = {"/behaviour/mouseScrollMultiplier",
                                          1.0};
    // BoolSetting twitchSeperateWriteConnection =
    // {"/behaviour/twitchSeperateWriteConnection", false};

    // Auto-completion
    BoolSetting onlyFetchChattersForSmallerStreamers = {
        "/behaviour/autocompletion/onlyFetchChattersForSmallerStreamers", true};
    IntSetting smallStreamerLimit = {
        "/behaviour/autocompletion/smallStreamerLimit", 1000};

    BoolSetting pauseChatHover = {"/behaviour/pauseChatHover", false};

    /// Commands
    BoolSetting allowCommandsAtEnd = {"/commands/allowCommandsAtEnd", false};

    /// Emotes
    BoolSetting scaleEmotesByLineHeight = {"/emotes/scaleEmotesByLineHeight",
                                           false};
    BoolSetting enableTwitchEmotes = {"/emotes/enableTwitchEmotes", true};
    BoolSetting enableBttvEmotes = {"/emotes/enableBTTVEmotes", true};
    BoolSetting enableFfzEmotes = {"/emotes/enableFFZEmotes", true};
    BoolSetting enableEmojis = {"/emotes/enableEmojis", true};
    BoolSetting enableGifAnimations = {"/emotes/enableGifAnimations", true};
    FloatSetting emoteScale = {"/emotes/scale", 1.f};

    QStringSetting emojiSet = {"/emotes/emojiSet", "EmojiOne 2"};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};
    BoolSetting enableLinkInfoTooltip = {"/links/linkInfoTooltip", false};
    BoolSetting enableUnshortLinks = {"/links/unshortLinks", false};
    BoolSetting enableLowercaseLink = {"/links/linkLowercase", true};

    /// Ignored phrases
    QStringSetting ignoredPhraseReplace = {"/ignore/ignoredPhraseReplace",
                                           "***"};

    /// Ingored Users
    BoolSetting enableTwitchIgnoredUsers = {"/ignore/enableTwitchIgnoredUsers",
                                            true};

    /// Moderation
    QStringSetting timeoutAction = {"/moderation/timeoutAction", "Disable"};

    /// Highlighting
    //    BoolSetting enableHighlights = {"/highlighting/enabled", true};
    BoolSetting customHighlightSound = {"/highlighting/useCustomSound", false};
    BoolSetting enableSelfHighlight = {
        "/highlighting/selfHighlight/nameIsHighlightKeyword", true};
    BoolSetting enableSelfHighlightSound = {
        "/highlighting/selfHighlight/enableSound", true};
    BoolSetting enableSelfHighlightTaskbar = {
        "/highlighting/selfHighlight/enableTaskbarFlashing", true};
    BoolSetting enableWhisperHighlight = {
        "/highlighting/whisperHighlight/whispersHighlighted", true};
    BoolSetting enableWhisperHighlightSound = {
        "/highlighting/whisperHighlight/enableSound", false};
    BoolSetting enableWhisperHighlightTaskbar = {
        "/highlighting/whisperHighlight/enableTaskbarFlashing", false};
    QStringSetting highlightColor = {"/highlighting/color", "#4B282C"};

    BoolSetting longAlerts = {"/highlighting/alerts", false};

    /// Logging
    BoolSetting enableLogging = {"/logging/enabled", false};

    QStringSetting logPath = {"/logging/path", ""};

    QStringSetting pathHighlightSound = {"/highlighting/highlightSoundPath",
                                         "qrc:/sounds/ping2.wav"};

    BoolSetting highlightAlwaysPlaySound = {"/highlighting/alwaysPlaySound",
                                            false};

    BoolSetting inlineWhispers = {"/whispers/enableInlineWhispers", true};

    /// Notifications
    BoolSetting notificationFlashTaskbar = {"/notifications/enableFlashTaskbar",
                                            false};
    BoolSetting notificationPlaySound = {"/notifications/enablePlaySound",
                                         false};
    BoolSetting notificationCustomSound = {"/notifications/customPlaySound",
                                           false};
    QStringSetting notificationPathSound = {"/notifications/highlightSoundPath",
                                            "qrc:/sounds/ping3.wav"};

    BoolSetting notificationToast = {"/notifications/enableToast", false};

    /// External tools
    // Streamlink
    BoolSetting streamlinkUseCustomPath = {"/external/streamlink/useCustomPath",
                                           false};
    QStringSetting streamlinkPath = {"/external/streamlink/customPath", ""};
    QStringSetting preferredQuality = {"/external/streamlink/quality",
                                       "Choose"};
    QStringSetting streamlinkOpts = {"/external/streamlink/options", ""};

    /// Misc
    IntSetting startUpNotification = {"/misc/startUpNotification", 0};
    QStringSetting currentVersion = {"/misc/currentVersion", ""};

    QStringSetting cachePath = {"/cache/path", ""};

    void saveSnapshot();
    void restoreSnapshot();

private:
    void updateModerationActions();

    std::unique_ptr<rapidjson::Document> snapshot_;
};

Settings *getSettings();

}  // namespace chatterino
