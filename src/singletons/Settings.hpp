#pragma once

#include "common/Channel.hpp"
#include "common/ChatterinoSetting.hpp"
#include "common/enums/MessageOverflow.hpp"
#include "common/Modes.hpp"
#include "common/SignalVector.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/logging/ChannelLog.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "controllers/sound/ISoundController.hpp"
#include "singletons/Toasts.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "widgets/Notebook.hpp"

#include <pajlada/settings/setting.hpp>
#include <pajlada/settings/settinglistener.hpp>
#include <pajlada/signals/signalholder.hpp>

using TimeoutButton = std::pair<QString, int>;

namespace chatterino {

class Args;

#ifdef Q_OS_WIN32
#    define DEFAULT_FONT_FAMILY "Segoe UI"
#    define DEFAULT_FONT_SIZE 10
#else
#    ifdef Q_OS_MACOS
#        define DEFAULT_FONT_FAMILY "Helvetica Neue"
#        define DEFAULT_FONT_SIZE 12
#    else
#        define DEFAULT_FONT_FAMILY "Arial"
#        define DEFAULT_FONT_SIZE 11
#    endif
#endif

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting);

enum UsernameDisplayMode : int {
    Username = 1,                  // Username
    LocalizedName = 2,             // Localized name
    UsernameAndLocalizedName = 3,  // Username (Localized name)
};

enum ThumbnailPreviewMode : int {
    DontShow = 0,

    AlwaysShow = 1,

    ShowOnShift = 2,
};

enum UsernameRightClickBehavior : int {
    Reply = 0,
    Mention = 1,
    Ignore = 2,
};

enum class ChatSendProtocol : int {
    Default = 0,
    IRC = 1,
    Helix = 2,
};

enum class ShowModerationState : int {
    // Always show this moderation-related item
    Always = 0,
    // Never show this moderation-related item
    Never = 1,
};

enum class StreamLinkPreferredQuality : std::uint8_t {
    Choose,
    Source,
    High,
    Medium,
    Low,
    AudioOnly,
};

enum StreamerModeSetting {
    Disabled = 0,
    Enabled = 1,
    DetectStreamingSoftware = 2,
};

/// Settings which are availlable for reading and writing on the gui thread.
// These settings are still accessed concurrently in the code but it is bad practice.
class Settings
{
    static Settings *instance_;
    Settings *prevInstance_ = nullptr;

    const bool disableSaving;

public:
    Settings(const Args &args, const QString &settingsDirectory);
    ~Settings();

    static Settings &instance();

    /// Request the settings to be saved to file
    ///
    /// Depending on the launch options, a save might end up not happening
    void requestSave() const;

    void saveSnapshot();
    void restoreSnapshot();

    FloatSetting uiScale = {"/appearance/uiScale2", 1};
    BoolSetting windowTopMost = {"/appearance/windowAlwaysOnTop", false};

    float getClampedUiScale() const;
    void setClampedUiScale(float value);

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
    EnumSetting<MessageOverflow> messageOverflow = {
        "/appearance/messages/messageOverflow", MessageOverflow::Highlight};
    BoolSetting separateMessages = {"/appearance/messages/separateMessages",
                                    false};
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
    EnumSetting<NotebookTabVisibility> tabVisibility = {
        "/appearance/tabVisibility",
        NotebookTabVisibility::AllTabs,
    };

    //    BoolSetting collapseLongMessages =
    //    {"/appearance/messages/collapseLongMessages", false};
    QStringSetting chatFontFamily{
        "/appearance/currentFontFamily",
        DEFAULT_FONT_FAMILY,
    };
    IntSetting chatFontSize{
        "/appearance/currentFontSize",
        DEFAULT_FONT_SIZE,
    };
    IntSetting chatFontWeight = {
        "/appearance/currentFontWeight",
        QFont::Normal,
    };
    BoolSetting hideReplyContext = {"/appearance/hideReplyContext", false};
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

    FloatSetting overlayScaleFactor = {"/appearance/overlay/scaleFactor", 1};
    IntSetting overlayBackgroundOpacity = {
        "/appearance/overlay/backgroundOpacity", 50};
    BoolSetting enableOverlayShadow = {"/appearance/overlay/shadow", true};
    IntSetting overlayShadowOpacity = {"/appearance/overlay/shadowOpacity",
                                       255};
    QStringSetting overlayShadowColor = {"/appearance/overlay/shadowColor",
                                         "#000"};
    // These should be floats, but there's no good input UI for them
    IntSetting overlayShadowOffsetX = {"/appearance/overlay/shadowOffsetX", 2};
    IntSetting overlayShadowOffsetY = {"/appearance/overlay/shadowOffsetY", 2};
    IntSetting overlayShadowRadius = {"/appearance/overlay/shadowRadius", 8};

    float getClampedOverlayScale() const;
    void setClampedOverlayScale(float value);

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
    QSizeSetting lastPopupSize = {
        "/appearance/lastPopup/size",
        {300, 500},
    };

    // Scrollbar
    BoolSetting hideScrollbarThumb = {
        "/appearance/scrollbar/hideThumb",
        false,
    };
    BoolSetting hideScrollbarHighlights = {
        "/appearance/scrollbar/hideHighlights",
        false,
    };

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

    EnumSetting<UsernameRightClickBehavior> usernameRightClickBehavior = {
        "/behaviour/usernameRightClickBehavior",
        UsernameRightClickBehavior::Mention,
    };
    EnumSetting<UsernameRightClickBehavior> usernameRightClickModifierBehavior =
        {
            "/behaviour/usernameRightClickBehaviorWithModifier",
            UsernameRightClickBehavior::Reply,
    };
    EnumSetting<Qt::KeyboardModifier> usernameRightClickModifier = {
        "/behaviour/usernameRightClickModifier",
        Qt::KeyboardModifier::ShiftModifier};

    BoolSetting autoSubToParticipatedThreads = {
        "/behaviour/autoSubToParticipatedThreads",
        true,
    };
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
    BoolSetting alwaysIncludeBroadcasterInUserCompletions = {
        "/behaviour/autocompletion/alwaysIncludeBroadcasterInUserCompletions",
        true,
    };
    BoolSetting useSmartEmoteCompletion = {
        "/experiments/useSmartEmoteCompletion",
        false,
    };

    FloatSetting pauseOnHoverDuration = {"/behaviour/pauseOnHoverDuration", 0};
    EnumSetting<Qt::KeyboardModifier> pauseChatModifier = {
        "/behaviour/pauseChatModifier", Qt::KeyboardModifier::NoModifier};
    BoolSetting autorun = {"/behaviour/autorun", false};
    BoolSetting mentionUsersWithComma = {"/behaviour/mentionUsersWithComma",
                                         true};

    /// Emotes
    BoolSetting scaleEmotesByLineHeight = {"/emotes/scaleEmotesByLineHeight",
                                           false};
    BoolSetting enableEmoteImages = {"/emotes/enableEmoteImages", true};
    BoolSetting animateEmotes = {"/emotes/enableGifAnimations", true};
    BoolSetting enableZeroWidthEmotes = {"/emotes/enableZeroWidthEmotes", true};
    FloatSetting emoteScale = {"/emotes/scale", 1.f};
    BoolSetting showUnlistedSevenTVEmotes = {
        "/emotes/showUnlistedSevenTVEmotes", false};
    QStringSetting emojiSet = {"/emotes/emojiSet", "Twitter"};

    BoolSetting stackBits = {"/emotes/stackBits", false};
    BoolSetting removeSpacesBetweenEmotes = {
        "/emotes/removeSpacesBetweenEmotes", false};

    BoolSetting enableBTTVGlobalEmotes = {"/emotes/bttv/global", true};
    BoolSetting enableBTTVChannelEmotes = {"/emotes/bttv/channel", true};
    BoolSetting enableBTTVLiveUpdates = {"/emotes/bttv/liveupdates", true};
    BoolSetting enableFFZGlobalEmotes = {"/emotes/ffz/global", true};
    BoolSetting enableFFZChannelEmotes = {"/emotes/ffz/channel", true};
    BoolSetting enableSevenTVGlobalEmotes = {"/emotes/seventv/global", true};
    BoolSetting enableSevenTVChannelEmotes = {"/emotes/seventv/channel", true};
    BoolSetting enableSevenTVEventAPI = {"/emotes/seventv/eventapi", true};
    BoolSetting sendSevenTVActivity = {"/emotes/seventv/sendActivity", true};

    /// Links
    BoolSetting linksDoubleClickOnly = {"/links/doubleClickToOpen", false};
    BoolSetting linkInfoTooltip = {"/links/linkInfoTooltip", false};
    IntSetting thumbnailSize = {"/appearance/thumbnailSize", 0};
    IntSetting thumbnailSizeStream = {"/appearance/thumbnailSizeStream", 2};
    BoolSetting unshortLinks = {"/links/unshortLinks", false};
    BoolSetting lowercaseDomains = {"/links/linkLowercase", true};

    /// Streamer Mode
    // TODO: Should these settings be converted to booleans that live outside of
    // streamer mode?
    // Something like:
    //  - "Hide when streamer mode is enabled"
    //  - "Always hide"
    //  - "Don't hide"
    EnumSetting<StreamerModeSetting> enableStreamerMode = {
        "/streamerMode/enabled", StreamerModeSetting::DetectStreamingSoftware};
    BoolSetting streamerModeHideUsercardAvatars = {
        "/streamerMode/hideUsercardAvatars", true};
    BoolSetting streamerModeHideLinkThumbnails = {
        "/streamerMode/hideLinkThumbnails", true};
    BoolSetting streamerModeHideViewerCountAndDuration = {
        "/streamerMode/hideViewerCountAndDuration", false};
    BoolSetting streamerModeHideModActions = {"/streamerMode/hideModActions",
                                              true};
    BoolSetting streamerModeHideRestrictedUsers = {
        "/streamerMode/hideRestrictedUsers",
        true,
    };
    BoolSetting streamerModeMuteMentions = {"/streamerMode/muteMentions", true};
    BoolSetting streamerModeSuppressLiveNotifications = {
        "/streamerMode/supressLiveNotifications", false};
    BoolSetting streamerModeSuppressInlineWhispers = {
        "/streamerMode/suppressInlineWhispers", true};
    BoolSetting streamerModeHideBlockedTermText = {
        "/streamerMode/hideBlockedTermText",
        true,
    };

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
    EnumStringSetting<ShowModerationState> showBlockedTermAutomodMessages = {
        "/moderation/showBlockedTermAutomodMessages",
        ShowModerationState::Always,
    };

    /// Highlighting
    //    BoolSetting enableHighlights = {"/highlighting/enabled", true};

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

    BoolSetting enableSelfMessageHighlight = {
        "/highlighting/selfMessageHighlight/enabled", false};
    BoolSetting showSelfMessageHighlightInMentions = {
        "/highlighting/selfMessageHighlight/showInMentions", false};
    QStringSetting selfMessageHighlightColor = {
        "/highlighting/selfMessageHighlight/color", ""};

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
    //    QStringSetting redeemedHighlightSoundUrl = {
    //        "/highlighting/redeemedHighlightSoundUrl", ""};
    QStringSetting redeemedHighlightColor = {
        "/highlighting/redeemedHighlightColor", ""};

    BoolSetting enableFirstMessageHighlight = {
        "/highlighting/firstMessageHighlight/highlighted", true};
    //    BoolSetting enableFirstMessageHighlightSound = {
    //        "/highlighting/firstMessageHighlight/enableSound", false};
    //    BoolSetting enableFirstMessageHighlightTaskbar = {
    //        "/highlighting/firstMessageHighlight/enableTaskbarFlashing", false};
    //    QStringSetting firstMessageHighlightSoundUrl = {
    //        "/highlighting/firstMessageHighlightSoundUrl", ""};
    QStringSetting firstMessageHighlightColor = {
        "/highlighting/firstMessageHighlightColor", ""};

    BoolSetting enableElevatedMessageHighlight = {
        "/highlighting/elevatedMessageHighlight/highlighted", true};
    //    BoolSetting enableElevatedMessageHighlightSound = {
    //        "/highlighting/elevatedMessageHighlight/enableSound", false};
    //    BoolSetting enableElevatedMessageHighlightTaskbar = {
    //        "/highlighting/elevatedMessageHighlight/enableTaskbarFlashing", false};
    //    QStringSetting elevatedMessageHighlightSoundUrl = {
    //        "/highlighting/elevatedMessageHighlight/soundUrl", ""};
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

    BoolSetting enableAutomodHighlight = {
        "/highlighting/automod/enabled",
        true,
    };
    BoolSetting showAutomodInMentions = {
        "/highlighting/automod/showInMentions",
        false,
    };
    BoolSetting enableAutomodHighlightSound = {
        "/highlighting/automod/enableSound",
        false,
    };
    BoolSetting enableAutomodHighlightTaskbar = {
        "/highlighting/automod/enableTaskbarFlashing",
        false,
    };
    QStringSetting automodHighlightSoundUrl = {
        "/highlighting/automod/soundUrl",
        "",
    };
    QStringSetting automodHighlightColor = {"/highlighting/automod/color", ""};

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
    BoolSetting onlyLogListedChannels = {"/logging/onlyLogListedChannels",
                                         false};
    BoolSetting separatelyStoreStreamLogs = {
        "/logging/separatelyStoreStreamLogs",
        false,
    };

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
    BoolSetting suppressInitialLiveNotification = {
        "/notifications/suppressInitialLive", false};

    BoolSetting notificationToast = {"/notifications/enableToast", false};
    BoolSetting createShortcutForToasts = {
        "/notifications/createShortcutForToasts",
        (Modes::instance().isPortable || Modes::instance().isExternallyPackaged)
            ? false
            : true,
    };
    IntSetting openFromToast = {"/notifications/openFromToast",
                                static_cast<int>(ToastReaction::OpenInBrowser)};

    /// External tools
    // Streamlink
    BoolSetting streamlinkUseCustomPath = {"/external/streamlink/useCustomPath",
                                           false};
    QStringSetting streamlinkPath = {"/external/streamlink/customPath", ""};
    EnumStringSetting<StreamLinkPreferredQuality> preferredQuality = {
        "/external/streamlink/quality",
        StreamLinkPreferredQuality::Choose,
    };
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

    IntSetting startUpNotification = {"/misc/startUpNotification", 0};
    QStringSetting currentVersion = {"/misc/currentVersion", ""};
    IntSetting overlayKnowledgeLevel = {"/misc/overlayKnowledgeLevel", 0};

    BoolSetting loadTwitchMessageHistoryOnConnect = {
        "/misc/twitch/loadMessageHistoryOnConnect", true};
    IntSetting twitchMessageHistoryLimit = {
        "/misc/twitch/messageHistoryLimit",
        800,
    };
    IntSetting scrollbackSplitLimit = {
        "/misc/scrollback/splitLimit",
        1000,
    };
    IntSetting scrollbackUsercardLimit = {
        "/misc/scrollback/usercardLimit",
        1000,
    };

    EnumStringSetting<ChatSendProtocol> chatSendProtocol = {
        "/misc/chatSendProtocol", ChatSendProtocol::Default};

    BoolSetting openLinksIncognito = {"/misc/openLinksIncognito", 0};

    EnumSetting<ThumbnailPreviewMode> emotesTooltipPreview = {
        "/misc/emotesTooltipPreview",
        ThumbnailPreviewMode::AlwaysShow,
    };
    QStringSetting cachePath = {"/cache/path", ""};
    BoolSetting attachExtensionToAnyProcess = {
        "/misc/attachExtensionToAnyProcess", false};
    BoolSetting askOnImageUpload = {"/misc/askOnImageUpload", true};
    BoolSetting informOnTabVisibilityToggle = {"/misc/askOnTabVisibilityToggle",
                                               true};
    BoolSetting lockNotebookLayout = {"/misc/lockNotebookLayout", false};
    BoolSetting showPronouns = {"/misc/showPronouns", false};

    /// UI

    BoolSetting showSendButton = {"/ui/showSendButton", false};

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

    BoolSetting pluginsEnabled = {"/plugins/supportEnabled", false};
    ChatterinoSetting<std::vector<QString>> enabledPlugins = {
        "/plugins/enabledPlugins", {}};

    // Advanced
    EnumStringSetting<SoundBackend> soundBackend = {
        "/sound/backend",
        SoundBackend::Miniaudio,
    };
    BoolSetting enableExperimentalEventSub = {
        "/eventsub/enableExperimental",
        true,
    };

    QStringSetting additionalExtensionIDs{"/misc/additionalExtensionIDs", ""};

private:
    ChatterinoSetting<std::vector<HighlightPhrase>> highlightedMessagesSetting =
        {"/highlighting/highlights"};
    ChatterinoSetting<std::vector<HighlightPhrase>> highlightedUsersSetting = {
        "/highlighting/users"};
    ChatterinoSetting<std::vector<HighlightBadge>> highlightedBadgesSetting = {
        "/highlighting/badges"};
    ChatterinoSetting<std::vector<HighlightBlacklistUser>>
        blacklistedUsersSetting = {"/highlighting/blacklist"};
    ChatterinoSetting<std::vector<IgnorePhrase>> ignoredMessagesSetting = {
        "/ignore/phrases"};
    ChatterinoSetting<std::vector<QString>> mutedChannelsSetting = {
        "/pings/muted"};
    ChatterinoSetting<std::vector<FilterRecordPtr>> filterRecordsSetting = {
        "/filtering/filters"};
    ChatterinoSetting<std::vector<Nickname>> nicknamesSetting = {"/nicknames"};
    ChatterinoSetting<std::vector<ModerationAction>> moderationActionsSetting =
        {"/moderation/actions"};
    ChatterinoSetting<std::vector<ChannelLog>> loggedChannelsSetting = {
        "/logging/channels"};
    SignalVector<QString> mutedChannels;

public:
    SignalVector<HighlightPhrase> highlightedMessages;
    SignalVector<HighlightPhrase> highlightedUsers;
    SignalVector<HighlightBadge> highlightedBadges;
    SignalVector<HighlightBlacklistUser> blacklistedUsers;
    SignalVector<IgnorePhrase> ignoredMessages;
    SignalVector<FilterRecordPtr> filterRecords;
    SignalVector<Nickname> nicknames;
    SignalVector<ModerationAction> moderationActions;
    SignalVector<ChannelLog> loggedChannels;

    bool isHighlightedUser(const QString &username);
    bool isBlacklistedUser(const QString &username);
    bool isMutedChannel(const QString &channelName);
    bool toggleMutedChannel(const QString &channelName);
    std::optional<QString> matchNickname(const QString &username);
    void mute(const QString &channelName);
    void unmute(const QString &channelName);

private:
    void updateModerationActions();

    std::unique_ptr<rapidjson::Document> snapshot_;

    pajlada::Signals::SignalHolder signalHolder;
};

Settings *getSettings();

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::StreamLinkPreferredQuality>(
        chatterino::StreamLinkPreferredQuality value) noexcept
{
    using chatterino::StreamLinkPreferredQuality;
    switch (value)
    {
        case chatterino::StreamLinkPreferredQuality::Choose:
        case chatterino::StreamLinkPreferredQuality::Source:
        case chatterino::StreamLinkPreferredQuality::High:
        case chatterino::StreamLinkPreferredQuality::Medium:
        case chatterino::StreamLinkPreferredQuality::Low:
            return default_tag;

        case chatterino::StreamLinkPreferredQuality::AudioOnly:
            return "Audio only";

        default:
            return default_tag;
    }
}
