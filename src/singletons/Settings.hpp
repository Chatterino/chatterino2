#pragma once

#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>

#include "BaseSettings.hpp"
#include "common/Channel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "singletons/Toasts.hpp"

namespace chatterino {

class Settings : public ABSettings
{
    static Settings *instance;

public:
    Settings(const QString &settingsDirectory);

    static Settings &getInstance();

    /// Appearance
    BoolSetting showTimestamps = {"/appearance/messages/showTimestamps", true};
    BoolSetting animationsWhenFocused = {
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
    BoolSetting compactEmotes = {"/appearance/messages/compactEmotes", true};
    BoolSetting hideModerated = {"/appearance/messages/hideModerated", false};
    BoolSetting hideModerationActions = {
        "/appearance/messages/hideModerationActions", false};
    BoolSetting colorizeNicknames = {"/appearance/messages/colorizeNicknames",
                                     false};

    //    BoolSetting collapseLongMessages =
    //    {"/appearance/messages/collapseLongMessages", false};
    IntSetting collpseMessagesMinLines = {
        "/appearance/messages/collapseMessagesMinLines", 0};
    BoolSetting alternateMessages = {
        "/appearance/messages/alternateMessageBackground", false};
    FloatSetting boldScale = {"/appearance/boldScale", 63};
    BoolSetting showTabCloseButton = {"/appearance/showTabCloseButton", true};
    BoolSetting showTabLive = {"/appearance/showTabLiveButton", false};
    BoolSetting hidePreferencesButton = {"/appearance/hidePreferencesButton",
                                         false};
    BoolSetting hideUserButton = {"/appearance/hideUserButton", false};
    BoolSetting enableSmoothScrolling = {"/appearance/smoothScrolling", true};
    BoolSetting enableSmoothScrollingNewMessages = {
        "/appearance/smoothScrollingNewMessages", false};
    BoolSetting boldUsernames = {"/appearance/messages/boldUsernames", false};
    // BoolSetting customizable splitheader
    BoolSetting headerViewerCount = {"/appearance/splitheader/showViewerCount",
                                     false};
    BoolSetting headerStreamTitle = {"/appearance/splitheader/showTitle",
                                     false};
    BoolSetting headerGame = {"/appearance/splitheader/showGame", false};
    BoolSetting headerUptime = {"/appearance/splitheader/showUptime", false};
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
    BoolSetting prefixOnlyEmoteCompletion = {
        "/behaviour/autocompletion/prefixOnlyCompletion", true};

    FloatSetting pauseOnHoverDuration = {"/behaviour/pauseOnHoverDuration", 0};
    EnumSetting<Qt::KeyboardModifier> pauseChatModifier = {
        "/behaviour/pauseChatModifier", Qt::KeyboardModifier::NoModifier};
    BoolSetting autorun = {"/behaviour/autorun", false};
    BoolSetting mentionUsersWithComma = {"/behaviour/mentionUsersWithComma",
                                         true};

    /// Commands
    BoolSetting allowCommandsAtEnd = {"/commands/allowCommandsAtEnd", false};

    /// Emotes
    BoolSetting scaleEmotesByLineHeight = {"/emotes/scaleEmotesByLineHeight",
                                           false};
    BoolSetting enableEmoteImages = {"/emotes/enableEmoteImages", true};
    BoolSetting animateEmotes = {"/emotes/enableGifAnimations", true};
    FloatSetting emoteScale = {"/emotes/scale", 1.f};

    QStringSetting emojiSet = {"/emotes/emojiSet", "EmojiOne 2"};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};
    BoolSetting linkInfoTooltip = {"/links/linkInfoTooltip", false};
    BoolSetting unshortLinks = {"/links/unshortLinks", false};
    BoolSetting lowercaseDomains = {"/links/linkLowercase", true};

    /// Ignored phrases
    QStringSetting ignoredPhraseReplace = {"/ignore/ignoredPhraseReplace",
                                           "***"};

    /// Ingored Users
    BoolSetting enableTwitchIgnoredUsers = {"/ignore/enableTwitchIgnoredUsers",
                                            true};
    IntSetting showIgnoredUsersMessages = {"/ignore/showIgnoredUsers", 0};

    /// Moderation
    QStringSetting timeoutAction = {"/moderation/timeoutAction", "Disable"};
    IntSetting timeoutStackStyle = {
        "/moderation/timeoutStackStyle",
        static_cast<int>(TimeoutStackStyle::Default)};

    /// Highlighting
    //    BoolSetting enableHighlights = {"/highlighting/enabled", true};
    BoolSetting customHighlightSound = {"/highlighting/useCustomSound", false};

    BoolSetting enableSelfHighlight = {
        "/highlighting/selfHighlight/nameIsHighlightKeyword", true};
    BoolSetting enableSelfHighlightSound = {
        "/highlighting/selfHighlight/enableSound", true};
    BoolSetting enableSelfHighlightTaskbar = {
        "/highlighting/selfHighlight/enableTaskbarFlashing", true};
    QStringSetting selfHighlightSoundUrl = {
        "/highlighting/selfHighlightSoundUrl", ""};
    QStringSetting selfHighlightColor = {"/highlighting/selfHighlightColor",
                                         ""};

    BoolSetting enableWhisperHighlight = {
        "/highlighting/whisperHighlight/whispersHighlighted", true};
    BoolSetting enableWhisperHighlightSound = {
        "/highlighting/whisperHighlight/enableSound", false};
    BoolSetting enableWhisperHighlightTaskbar = {
        "/highlighting/whisperHighlight/enableTaskbarFlashing", false};
    QStringSetting whisperHighlightSoundUrl = {
        "/highlighting/whisperHighlightSoundUrl", ""};
    QStringSetting whisperHighlightColor = {
        "/highlighting/whisperHighlightColor", ""};

    BoolSetting enableSubHighlight = {
        "/highlighting/subHighlight/subsHighlighted", true};
    BoolSetting enableSubHighlightSound = {
        "/highlighting/subHighlight/enableSound", false};
    BoolSetting enableSubHighlightTaskbar = {
        "/highlighting/subHighlight/enableTaskbarFlashing", false};
    QStringSetting subHighlightSoundUrl = {"/highlighting/subHighlightSoundUrl",
                                           ""};
    QStringSetting subHighlightColor = {"/highlighting/subHighlightColor", ""};

    QStringSetting highlightColor = {"/highlighting/color", ""};

    BoolSetting longAlerts = {"/highlighting/alerts", false};

    /// Logging
    BoolSetting enableLogging = {"/logging/enabled", false};

    QStringSetting logPath = {"/logging/path", ""};

    QStringSetting pathHighlightSound = {"/highlighting/highlightSoundPath",
                                         ""};

    BoolSetting highlightAlwaysPlaySound = {"/highlighting/alwaysPlaySound",
                                            false};

    BoolSetting inlineWhispers = {"/whispers/enableInlineWhispers", true};
    BoolSetting highlightInlineWhispers = {"/whispers/highlightInlineWhispers",
                                           false};

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
    IntSetting openFromToast = {"/notifications/openFromToast",
                                static_cast<int>(ToastReaction::OpenInBrowser)};

    /// External tools
    // Streamlink
    BoolSetting streamlinkUseCustomPath = {"/external/streamlink/useCustomPath",
                                           false};
    QStringSetting streamlinkPath = {"/external/streamlink/customPath", ""};
    QStringSetting preferredQuality = {"/external/streamlink/quality",
                                       "Choose"};
    QStringSetting streamlinkOpts = {"/external/streamlink/options", ""};

    /// Misc
    BoolSetting betaUpdates = {"/misc/beta", false};
#ifdef Q_OS_LINUX
    BoolSetting useKeyring = {"/misc/useKeyring", true};
#endif
    BoolSetting enableExperimentalIrc = {"/misc/experimentalIrc", false};

    IntSetting startUpNotification = {"/misc/startUpNotification", 0};
    QStringSetting currentVersion = {"/misc/currentVersion", ""};
    BoolSetting loadTwitchMessageHistoryOnConnect = {
        "/misc/twitch/loadMessageHistoryOnConnect", true};
    IntSetting emotesTooltipPreview = {"/misc/emotesTooltipPreview", 0};
    BoolSetting openLinksIncognito = {"/misc/openLinksIncognito", 0};

    QStringSetting cachePath = {"/cache/path", ""};
    BoolSetting restartOnCrash = {"/misc/restartOnCrash", true};

    /// Debug
    BoolSetting showUnhandledIrcMessages = {"/debug/showUnhandledIrcMessages",
                                            false};

    /// UI
    // Purely QOL settings are here (like last item in a list).
    IntSetting lastSelectChannelTab = {"/ui/lastSelectChannelTab", 0};
    IntSetting lastSelectIrcConn = {"/ui/lastSelectIrcConn", 0};

private:
    void updateModerationActions();
};

}  // namespace chatterino

#ifdef CHATTERINO
#    include "singletons/Settings.hpp"
#endif
