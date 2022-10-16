#pragma once

#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>

#include "BaseSettings.hpp"
#include "common/Channel.hpp"
#include "common/SignalVector.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "singletons/Toasts.hpp"
#include "util/StreamerMode.hpp"
#include "widgets/Notebook.hpp"

using TimeoutButton = std::pair<QString, int>;

namespace chatterino {

class HighlightPhrase;
class HighlightBlacklistUser;
class IgnorePhrase;
class FilterRecord;
class Nickname;

/// Settings which are available for reading on all threads.
class ConcurrentSettings
{
public:
    ConcurrentSettings();

    SignalVector<HighlightPhrase> &highlightedMessages;
    SignalVector<HighlightPhrase> &highlightedUsers;
    SignalVector<HighlightBadge> &highlightedBadges;
    SignalVector<HighlightBlacklistUser> &blacklistedUsers;
    SignalVector<IgnorePhrase> &ignoredMessages;
    SignalVector<QString> &mutedChannels;
    SignalVector<FilterRecordPtr> &filterRecords;
    SignalVector<Nickname> &nicknames;
    SignalVector<ModerationAction> &moderationActions;

    bool isHighlightedUser(const QString &username);
    bool isBlacklistedUser(const QString &username);
    bool isMutedChannel(const QString &channelName);
    bool toggleMutedChannel(const QString &channelName);

private:
    void mute(const QString &channelName);
    void unmute(const QString &channelName);
};

ConcurrentSettings &getCSettings();

enum UsernameDisplayMode : int {
    Username = 1,                  // Username
    LocalizedName = 2,             // Localized name
    UsernameAndLocalizedName = 3,  // Username (Localized name)
};

enum HelixTimegateOverride : int {
    // Use the default timegated behaviour
    // This means we use the old IRC command up until the migration date and
    // switch over to the Helix API only after the migration date
    Timegate = 1,

    // Ignore timegating and always force use the IRC command
    AlwaysUseIRC = 2,

    // Ignore timegating and always force use the Helix API
    AlwaysUseHelix = 3,
};

/// Settings which are availlable for reading and writing on the gui thread.
// These settings are still accessed concurrently in the code but it is bad practice.
class Settings : public ABSettings, public ConcurrentSettings
{
    static Settings *instance_;

public:
    Settings(const QString &settingsDirectory);

    static Settings &instance();

    /// Appearance
    BoolSetting showTimestamps = {"/appearance/messages/showTimestamps", true};
    BoolSetting animationsWhenFocused = {
        "/appearance/enableAnimationsWhenFocused", false};
    QStringSetting timestampFormat = {"/appearance/messages/timestampFormat",
                                      "h:mm"};
    BoolSetting showLastMessageIndicator = {
        "/appearance/messages/showLastMessageIndicator", false};
    EnumSetting<Qt::BrushStyle> lastMessagePattern = {
        "/appearance/messages/lastMessagePattern", Qt::SolidPattern};
    QStringSetting lastMessageColor = {"/appearance/messages/lastMessageColor",
                                       "#7f2026"};
    BoolSetting showEmptyInput = {"/appearance/showEmptyInputBox", true};
    BoolSetting showMessageLength = {"/appearance/messages/showMessageLength",
                                     false};
    BoolSetting separateMessages = {"/appearance/messages/separateMessages",
                                    false};
    BoolSetting compactEmotes = {"/appearance/messages/compactEmotes", true};
    BoolSetting hideModerated = {"/appearance/messages/hideModerated", false};
    BoolSetting hideModerationActions = {
        "/appearance/messages/hideModerationActions", false};
    BoolSetting hideDeletionActions = {
        "/appearance/messages/hideDeletionActions", false};
    BoolSetting colorizeNicknames = {"/appearance/messages/colorizeNicknames",
                                     true};
    EnumSetting<UsernameDisplayMode> usernameDisplayMode = {
        "/appearance/messages/usernameDisplayMode",
        UsernameDisplayMode::UsernameAndLocalizedName};

    EnumSetting<NotebookTabLocation> tabDirection = {"/appearance/tabDirection",
                                                     NotebookTabLocation::Top};

    //    BoolSetting collapseLongMessages =
    //    {"/appearance/messages/collapseLongMessages", false};
    BoolSetting showReplyButton = {"/appearance/showReplyButton", false};
    BoolSetting stripReplyMention = {"/appearance/stripReplyMention", true};
    IntSetting collpseMessagesMinLines = {
        "/appearance/messages/collapseMessagesMinLines", 0};
    BoolSetting alternateMessages = {
        "/appearance/messages/alternateMessageBackground", false};
    FloatSetting boldScale = {"/appearance/boldScale", 63};
    BoolSetting showTabCloseButton = {"/appearance/showTabCloseButton", true};
    BoolSetting showTabLive = {"/appearance/showTabLiveButton", true};
    BoolSetting hidePreferencesButton = {"/appearance/hidePreferencesButton",
                                         false};
    BoolSetting hideUserButton = {"/appearance/hideUserButton", false};
    BoolSetting enableSmoothScrolling = {"/appearance/smoothScrolling", true};
    BoolSetting enableSmoothScrollingNewMessages = {
        "/appearance/smoothScrollingNewMessages", false};
    BoolSetting boldUsernames = {"/appearance/messages/boldUsernames", true};
    BoolSetting colorUsernames = {"/appearance/messages/colorUsernames", true};
    BoolSetting findAllUsernames = {"/appearance/messages/findAllUsernames",
                                    false};
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
    BoolSetting showBadgesPredictions = {"/appearance/badges/predictions",
                                         true};
    BoolSetting showBadgesChannelAuthority = {
        "/appearance/badges/ChannelAuthority", true};
    BoolSetting showBadgesSubscription = {"/appearance/badges/subscription",
                                          true};
    BoolSetting showBadgesVanity = {"/appearance/badges/vanity", true};
    BoolSetting showBadgesChatterino = {"/appearance/badges/chatterino", true};
    BoolSetting showBadgesFfz = {"/appearance/badges/ffz", true};
    BoolSetting useCustomFfzModeratorBadges = {
        "/appearance/badges/useCustomFfzModeratorBadges", true};
    BoolSetting useCustomFfzVipBadges = {
        "/appearance/badges/useCustomFfzVipBadges", true};
    BoolSetting showBadgesSevenTV = {"/appearance/badges/seventv", true};

    /// Behaviour
    BoolSetting allowDuplicateMessages = {"/behaviour/allowDuplicateMessages",
                                          true};
    BoolSetting mentionUsersWithAt = {"/behaviour/mentionUsersWithAt", false};
    BoolSetting showJoins = {"/behaviour/showJoins", false};
    BoolSetting showParts = {"/behaviour/showParts", false};
    FloatSetting mouseScrollMultiplier = {"/behaviour/mouseScrollMultiplier",
                                          1.0};
    BoolSetting autoCloseUserPopup = {"/behaviour/autoCloseUserPopup", true};
    BoolSetting autoCloseThreadPopup = {"/behaviour/autoCloseThreadPopup",
                                        false};
    // BoolSetting twitchSeperateWriteConnection =
    // {"/behaviour/twitchSeperateWriteConnection", false};

    // Auto-completion
    BoolSetting onlyFetchChattersForSmallerStreamers = {
        "/behaviour/autocompletion/onlyFetchChattersForSmallerStreamers", true};
    IntSetting smallStreamerLimit = {
        "/behaviour/autocompletion/smallStreamerLimit", 1000};
    BoolSetting prefixOnlyEmoteCompletion = {
        "/behaviour/autocompletion/prefixOnlyCompletion", true};
    BoolSetting userCompletionOnlyWithAt = {
        "/behaviour/autocompletion/userCompletionOnlyWithAt", false};
    BoolSetting emoteCompletionWithColon = {
        "/behaviour/autocompletion/emoteCompletionWithColon", true};
    BoolSetting showUsernameCompletionMenu = {
        "/behaviour/autocompletion/showUsernameCompletionMenu", true};

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
    BoolSetting showUnlistedSevenTVEmotes = {
        "/emotes/showUnlistedSevenTVEmotes", false};

    QStringSetting emojiSet = {"/emotes/emojiSet", "Twitter"};

    BoolSetting stackBits = {"/emotes/stackBits", false};
    BoolSetting removeSpacesBetweenEmotes = {
        "/emotes/removeSpacesBetweenEmotes", false};

    BoolSetting enableBTTVGlobalEmotes = {"/emotes/bttv/global", true};
    BoolSetting enableBTTVChannelEmotes = {"/emotes/bttv/channel", true};
    BoolSetting enableFFZGlobalEmotes = {"/emotes/ffz/global", true};
    BoolSetting enableFFZChannelEmotes = {"/emotes/ffz/channel", true};
    BoolSetting enableSevenTVGlobalEmotes = {"/emotes/seventv/global", true};
    BoolSetting enableSevenTVChannelEmotes = {"/emotes/seventv/channel", true};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};
    BoolSetting linkInfoTooltip = {"/links/linkInfoTooltip", false};
    IntSetting thumbnailSize = {"/appearance/thumbnailSize", 0};
    IntSetting thumbnailSizeStream = {"/appearance/thumbnailSizeStream", 2};
    BoolSetting unshortLinks = {"/links/unshortLinks", false};
    BoolSetting lowercaseDomains = {"/links/linkLowercase", true};

    /// Streamer Mode
    EnumSetting<StreamerModeSetting> enableStreamerMode = {
        "/streamerMode/enabled", StreamerModeSetting::DetectStreamingSoftware};
    BoolSetting streamerModeHideUsercardAvatars = {
        "/streamerMode/hideUsercardAvatars", true};
    BoolSetting streamerModeHideLinkThumbnails = {
        "/streamerMode/hideLinkThumbnails", true};
    BoolSetting streamerModeHideViewerCountAndDuration = {
        "/streamerMode/hideViewerCountAndDuration", false};
    BoolSetting streamerModeMuteMentions = {"/streamerMode/muteMentions", true};
    BoolSetting streamerModeSuppressLiveNotifications = {
        "/streamerMode/supressLiveNotifications", false};

    /// Ignored Phrases
    QStringSetting ignoredPhraseReplace = {"/ignore/ignoredPhraseReplace",
                                           "***"};

    /// Blocked Users
    BoolSetting enableTwitchBlockedUsers = {"/ignore/enableTwitchBlockedUsers",
                                            true};
    IntSetting showBlockedUsersMessages = {"/ignore/showBlockedUsers", 0};

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
    BoolSetting showSelfHighlightInMentions = {
        "/highlighting/selfHighlight/showSelfHighlightInMentions", true};
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

    BoolSetting enableRedeemedHighlight = {
        "/highlighting/redeemedHighlight/highlighted", true};
    //    BoolSetting enableRedeemedHighlightSound = {
    //        "/highlighting/redeemedHighlight/enableSound", false};
    //    BoolSetting enableRedeemedHighlightTaskbar = {
    //        "/highlighting/redeemedHighlight/enableTaskbarFlashing", false};
    QStringSetting redeemedHighlightSoundUrl = {
        "/highlighting/redeemedHighlightSoundUrl", ""};
    QStringSetting redeemedHighlightColor = {
        "/highlighting/redeemedHighlightColor", ""};

    BoolSetting enableFirstMessageHighlight = {
        "/highlighting/firstMessageHighlight/highlighted", true};
    //    BoolSetting enableFirstMessageHighlightSound = {
    //        "/highlighting/firstMessageHighlight/enableSound", false};
    //    BoolSetting enableFirstMessageHighlightTaskbar = {
    //        "/highlighting/firstMessageHighlight/enableTaskbarFlashing", false};
    QStringSetting firstMessageHighlightSoundUrl = {
        "/highlighting/firstMessageHighlightSoundUrl", ""};
    QStringSetting firstMessageHighlightColor = {
        "/highlighting/firstMessageHighlightColor", ""};

    BoolSetting enableElevatedMessageHighlight = {
        "/highlighting/elevatedMessageHighlight/highlighted", true};
    //    BoolSetting enableElevatedMessageHighlightSound = {
    //        "/highlighting/elevatedMessageHighlight/enableSound", false};
    //    BoolSetting enableElevatedMessageHighlightTaskbar = {
    //        "/highlighting/elevatedMessageHighlight/enableTaskbarFlashing", false};
    QStringSetting elevatedMessageHighlightSoundUrl = {
        "/highlighting/elevatedMessageHighlight/soundUrl", ""};
    QStringSetting elevatedMessageHighlightColor = {
        "/highlighting/elevatedMessageHighlight/color", ""};

    BoolSetting enableSubHighlight = {
        "/highlighting/subHighlight/subsHighlighted", true};
    BoolSetting enableSubHighlightSound = {
        "/highlighting/subHighlight/enableSound", false};
    BoolSetting enableSubHighlightTaskbar = {
        "/highlighting/subHighlight/enableTaskbarFlashing", false};
    QStringSetting subHighlightSoundUrl = {"/highlighting/subHighlightSoundUrl",
                                           ""};
    QStringSetting subHighlightColor = {"/highlighting/subHighlightColor", ""};

    BoolSetting enableThreadHighlight = {
        "/highlighting/thread/nameIsHighlightKeyword", true};
    BoolSetting showThreadHighlightInMentions = {
        "/highlighting/thread/showSelfHighlightInMentions", true};
    BoolSetting enableThreadHighlightSound = {
        "/highlighting/thread/enableSound", true};
    BoolSetting enableThreadHighlightTaskbar = {
        "/highlighting/thread/enableTaskbarFlashing", true};
    QStringSetting threadHighlightSoundUrl = {
        "/highlighting/threadHighlightSoundUrl", ""};
    QStringSetting threadHighlightColor = {"/highlighting/threadHighlightColor",
                                           ""};

    QStringSetting highlightColor = {"/highlighting/color", ""};

    BoolSetting longAlerts = {"/highlighting/alerts", false};

    BoolSetting highlightMentions = {"/highlighting/mentions", true};

    /// Filtering
    BoolSetting excludeUserMessagesFromFilter = {
        "/filtering/excludeUserMessages", false};

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
    BoolSetting notificationOnAnyChannel = {"/notifications/onAnyChannel",
                                            false};

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

    // Custom URI Scheme
    QStringSetting customURIScheme = {"/external/urischeme"};

    // Image Uploader
    BoolSetting imageUploaderEnabled = {"/external/imageUploader/enabled",
                                        false};
    QStringSetting imageUploaderUrl = {"/external/imageUploader/url", ""};
    QStringSetting imageUploaderFormField = {
        "/external/imageUploader/formField", ""};
    QStringSetting imageUploaderHeaders = {"/external/imageUploader/headers",
                                           ""};
    QStringSetting imageUploaderLink = {"/external/imageUploader/link", ""};
    QStringSetting imageUploaderDeletionLink = {
        "/external/imageUploader/deletionLink", ""};

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
    IntSetting twitchMessageHistoryLimit = {
        "/misc/twitch/messageHistoryLimit",
        800,
    };

    // Temporary time-gate-overrides
    EnumSetting<HelixTimegateOverride> helixTimegateRaid = {
        "/misc/twitch/helix-timegate/raid",
        HelixTimegateOverride::Timegate,
    };
    EnumSetting<HelixTimegateOverride> helixTimegateWhisper = {
        "/misc/twitch/helix-timegate/whisper",
        HelixTimegateOverride::Timegate,
    };
    EnumSetting<HelixTimegateOverride> helixTimegateVIPs = {
        "/misc/twitch/helix-timegate/vips",
        HelixTimegateOverride::Timegate,
    };

    IntSetting emotesTooltipPreview = {"/misc/emotesTooltipPreview", 1};
    BoolSetting openLinksIncognito = {"/misc/openLinksIncognito", 0};

    QStringSetting cachePath = {"/cache/path", ""};
    BoolSetting restartOnCrash = {"/misc/restartOnCrash", false};
    BoolSetting attachExtensionToAnyProcess = {
        "/misc/attachExtensionToAnyProcess", false};
    BoolSetting askOnImageUpload = {"/misc/askOnImageUpload", true};
    BoolSetting informOnTabVisibilityToggle = {"/misc/askOnTabVisibilityToggle",
                                               true};
    BoolSetting lockNotebookLayout = {"/misc/lockNotebookLayout", false};

    /// Debug
    BoolSetting showUnhandledIrcMessages = {"/debug/showUnhandledIrcMessages",
                                            false};

    /// UI
    // Purely QOL settings are here (like last item in a list).
    IntSetting lastSelectChannelTab = {"/ui/lastSelectChannelTab", 0};
    IntSetting lastSelectIrcConn = {"/ui/lastSelectIrcConn", 0};

    // Similarity
    BoolSetting similarityEnabled = {"/similarity/similarityEnabled", false};
    BoolSetting colorSimilarDisabled = {"/similarity/colorSimilarDisabled",
                                        true};
    BoolSetting hideSimilar = {"/similarity/hideSimilar", false};
    BoolSetting hideSimilarBySameUser = {"/similarity/hideSimilarBySameUser",
                                         true};
    BoolSetting hideSimilarMyself = {"/similarity/hideSimilarMyself", false};
    BoolSetting shownSimilarTriggerHighlights = {
        "/similarity/shownSimilarTriggerHighlights", false};
    FloatSetting similarityPercentage = {"/similarity/similarityPercentage",
                                         0.9f};
    IntSetting hideSimilarMaxDelay = {"/similarity/hideSimilarMaxDelay", 5};
    IntSetting hideSimilarMaxMessagesToCheck = {
        "/similarity/hideSimilarMaxMessagesToCheck", 3};

    /// Timeout buttons

    ChatterinoSetting<std::vector<TimeoutButton>> timeoutButtons = {
        "/timeouts/timeoutButtons",
        {{"s", 1},
         {"s", 30},
         {"m", 1},
         {"m", 5},
         {"m", 30},
         {"h", 1},
         {"d", 1},
         {"w", 1}}};

private:
    void updateModerationActions();
};

}  // namespace chatterino

#ifdef CHATTERINO
#    include "singletons/Settings.hpp"
#endif
