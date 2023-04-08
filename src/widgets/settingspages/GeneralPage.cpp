#include "widgets/settingspages/GeneralPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/NativeMessaging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/FuzzyConvert.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/StreamerMode.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/splits/SplitInput.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#define CHROME_EXTENSION_LINK                                           \
    "https://chrome.google.com/webstore/detail/chatterino-native-host/" \
    "glknmaideaikkmemifbfkhnomoknepka"
#define FIREFOX_EXTENSION_LINK \
    "https://addons.mozilla.org/en-US/firefox/addon/chatterino-native-host/"

// define to highlight sections in editor
#define addTitle addTitle
#define addSubtitle addSubtitle

#ifdef Q_OS_WIN
#    define META_KEY "Windows"
#else
#    define META_KEY "Meta"
#endif

namespace chatterino {
namespace {
    void addKeyboardModifierSetting(GeneralPageView &layout,
                                    const QString &title,
                                    EnumSetting<Qt::KeyboardModifier> &setting)
    {
        layout.addDropdown<std::underlying_type<Qt::KeyboardModifier>::type>(
            title, {"None", "Shift", "Control", "Alt", META_KEY}, setting,
            [](int index) {
                switch (index)
                {
                    case Qt::ShiftModifier:
                        return 1;
                    case Qt::ControlModifier:
                        return 2;
                    case Qt::AltModifier:
                        return 3;
                    case Qt::MetaModifier:
                        return 4;
                    default:
                        return 0;
                }
            },
            [](DropdownArgs args) {
                switch (args.index)
                {
                    case 1:
                        return Qt::ShiftModifier;
                    case 2:
                        return Qt::ControlModifier;
                    case 3:
                        return Qt::AltModifier;
                    case 4:
                        return Qt::MetaModifier;
                    default:
                        return Qt::NoModifier;
                }
            },
            false);
    }
}  // namespace

GeneralPage::GeneralPage()
{
    auto y = new QVBoxLayout;
    auto x = new QHBoxLayout;
    auto view = new GeneralPageView;
    this->view_ = view;
    x->addWidget(view);
    auto z = new QFrame;
    z->setLayout(x);
    y->addWidget(z);
    this->setLayout(y);

    this->initLayout(*view);

    this->initExtra();
}

bool GeneralPage::filterElements(const QString &query)
{
    if (this->view_)
        return this->view_->filterElements(query) || query.isEmpty();
    else
        return false;
}

void GeneralPage::initLayout(GeneralPageView &layout)
{
    auto &s = *getSettings();

    layout.addTitle("Interface");
    layout.addDropdown("Theme", {"White", "Light", "Dark", "Black"},
                       getApp()->themes->themeName);
    layout.addDropdown<QString>(
        "Font", {"Segoe UI", "Arial", "Choose..."},
        getApp()->fonts->chatFontFamily,
        [](auto val) {
            return val;
        },
        [this](auto args) {
            return this->getFont(args);
        });
    layout.addDropdown<int>(
        "Font size", {"9pt", "10pt", "12pt", "14pt", "16pt", "20pt"},
        getApp()->fonts->chatFontSize,
        [](auto val) {
            return QString::number(val) + "pt";
        },
        [](auto args) {
            return fuzzyToInt(args.value, 10);
        });
    layout.addDropdown<float>(
        "Zoom",
        {"0.5x", "0.6x", "0.7x", "0.8x", "0.9x", "Default", "1.2x", "1.4x",
         "1.6x", "1.8x", "2x", "2.33x", "2.66x", "3x", "3.5x", "4x"},
        s.uiScale,
        [](auto val) {
            if (val == 1)
                return QString("Default");
            else
                return QString::number(val) + "x";
        },
        [](auto args) {
            return fuzzyToFloat(args.value, 1.f);
        });
    ComboBox *tabDirectionDropdown =
        layout.addDropdown<std::underlying_type<NotebookTabLocation>::type>(
            "Tab layout", {"Top", "Left", "Right", "Bottom"}, s.tabDirection,
            [](auto val) {
                switch (val)
                {
                    case NotebookTabLocation::Top:
                        return "Top";
                    case NotebookTabLocation::Left:
                        return "Left";
                    case NotebookTabLocation::Right:
                        return "Right";
                    case NotebookTabLocation::Bottom:
                        return "Bottom";
                }

                return "";
            },
            [](auto args) {
                if (args.value == "Bottom")
                {
                    return NotebookTabLocation::Bottom;
                }
                else if (args.value == "Left")
                {
                    return NotebookTabLocation::Left;
                }
                else if (args.value == "Right")
                {
                    return NotebookTabLocation::Right;
                }
                else
                {
                    // default to top
                    return NotebookTabLocation::Top;
                }
            },
            false);
    tabDirectionDropdown->setMinimumWidth(
        tabDirectionDropdown->minimumSizeHint().width());

    layout.addCheckbox(
        "Show message reply context", s.hideReplyContext, true,
        "This setting will only affect how messages are shown. You can reply "
        "to a message regardless of this setting.");
    layout.addCheckbox("Show message reply button", s.showReplyButton, false,
                       "Show a reply button next to every chat message");

    auto removeTabSeq = getApp()->hotkeys->getDisplaySequence(
        HotkeyCategory::Window, "removeTab");
    QString removeTabShortcut = "an assigned hotkey (Window -> remove tab)";
    if (!removeTabSeq.isEmpty())
    {
        removeTabShortcut =
            removeTabSeq.toString(QKeySequence::SequenceFormat::NativeText);
    }
    layout.addCheckbox(
        "Show tab close button", s.showTabCloseButton, false,
        "When disabled, the x to close a tab will be hidden.\nTabs can still "
        "be closed by right-clicking or pressing " +
            removeTabShortcut + ".");
    layout.addCheckbox("Always on top", s.windowTopMost, false,
                       "Always keep Chatterino as the top window.");
#ifdef USEWINSDK
    layout.addCheckbox("Start with Windows", s.autorun, false,
                       "Start Chatterino when your computer starts.");
#endif
    if (!BaseWindow::supportsCustomWindowFrame())
    {
        auto settingsSeq = getApp()->hotkeys->getDisplaySequence(
            HotkeyCategory::Window, "openSettings");
        QString shortcut = " (no key bound to open them otherwise)";
        // TODO: maybe prevent the user from locking themselves out of the settings?
        if (!settingsSeq.isEmpty())
        {
            shortcut = QStringLiteral(" (%1 to show)")
                           .arg(settingsSeq.toString(
                               QKeySequence::SequenceFormat::NativeText));
        }
        layout.addCheckbox("Show preferences button" + shortcut,
                           s.hidePreferencesButton, true);
        layout.addCheckbox("Show user button", s.hideUserButton, true);
    }
    layout.addCheckbox("Mark tabs with live channels", s.showTabLive, false,
                       "Shows a red dot in the top right corner of a tab to "
                       "indicate one of the channels in the tab is live.");

    layout.addTitle("Chat");

    layout.addDropdown<float>(
        "Pause on mouse hover",
        {"Disabled", "0.5s", "1s", "2s", "5s", "Indefinite"},
        s.pauseOnHoverDuration,
        [](auto val) {
            if (val < -0.5f)
                return QString("Indefinite");
            else if (val < 0.001f)
                return QString("Disabled");
            else
                return QString::number(val) + "s";
        },
        [](auto args) {
            if (args.index == 0)
                return 0.0f;
            else if (args.value == "Indefinite")
                return -1.0f;
            else
                return fuzzyToFloat(args.value,
                                    std::numeric_limits<float>::infinity());
        });
    addKeyboardModifierSetting(layout, "Pause while holding a key",
                               s.pauseChatModifier);
    layout.addDropdown<float>(
        "Mousewheel scroll speed", {"0.5x", "0.75x", "Default", "1.5x", "2x"},
        s.mouseScrollMultiplier,
        [](auto val) {
            if (val == 1)
                return QString("Default");
            else
                return QString::number(val) + "x";
        },
        [](auto args) {
            return fuzzyToFloat(args.value, 1.f);
        });
    layout.addCheckbox("Smooth scrolling", s.enableSmoothScrolling);
    layout.addCheckbox("Smooth scrolling on new messages",
                       s.enableSmoothScrollingNewMessages);
    layout.addCheckbox("Show input when it's empty", s.showEmptyInput, false,
                       "Show the chat box even when there is nothing typed.");
    layout.addCheckbox(
        "Show message length while typing", s.showMessageLength, false,
        "Show how many characters are currently in your input box.\n"
        "Useful for making sure you don't go past the 500 character Twitch "
        "limit, or a lower limit enforced by a moderation bot");
    layout.addCheckbox(
        "Allow sending duplicate messages", s.allowDuplicateMessages, false,
        "Allow a single message to be repeatedly sent without any changes.");
    layout.addDropdown<std::underlying_type<MessageOverflow>::type>(
        "Message overflow", {"Highlight", "Prevent", "Allow"},
        s.messageOverflow,
        [](auto index) {
            return index;
        },
        [](auto args) {
            return static_cast<MessageOverflow>(args.index);
        },
        false,
        "Specify how Chatterino will handle messages that exceed Twitch "
        "message limits");

    layout.addTitle("Messages");
    layout.addCheckbox(
        "Separate with lines", s.separateMessages, false,
        "Adds a line inbetween each message to help better tell them apart.");
    layout.addCheckbox("Alternate background color", s.alternateMessages, false,
                       "Slightly change the background behind every other "
                       "message to help better tell them apart.");
    layout.addCheckbox("Hide deleted messages", s.hideModerated, false,
                       "When enabled, messages deleted by moderators will "
                       "be hidden.");
    layout.addDropdown<QString>(
        "Timestamp format",
        {"Disable", "h:mm", "hh:mm", "h:mm a", "hh:mm a", "h:mm:ss", "hh:mm:ss",
         "h:mm:ss a", "hh:mm:ss a", "h:mm:ss.zzz", "h:mm:ss.zzz a",
         "hh:mm:ss.zzz", "hh:mm:ss.zzz a"},
        s.timestampFormat,
        [](auto val) {
            return getSettings()->showTimestamps.getValue()
                       ? val
                       : QString("Disable");
        },
        [](auto args) {
            getSettings()->showTimestamps.setValue(args.index != 0);

            return args.index == 0 ? getSettings()->timestampFormat.getValue()
                                   : args.value;
        },
        true, "a = am/pm, zzz = milliseconds");
    layout.addDropdown<int>(
        "Limit message height",
        {"Never", "2 lines", "3 lines", "4 lines", "5 lines"},
        s.collpseMessagesMinLines,
        [](auto val) {
            return val ? QString::number(val) + " lines" : QString("Never");
        },
        [](auto args) {
            return fuzzyToInt(args.value, 0);
        });
    layout.addSeperator();
    layout.addCheckbox("Draw a line below the most recent message before "
                       "switching applications.",
                       s.showLastMessageIndicator, false,
                       "Adds an underline below the most recent message "
                       "sent before you tabbed out of Chatterino.");
    layout.addDropdown<std::underlying_type<Qt::BrushStyle>::type>(
        "Line style", {"Dotted", "Solid"}, s.lastMessagePattern,
        [](int value) {
            switch (value)
            {
                case Qt::VerPattern:
                    return 0;
                case Qt::SolidPattern:
                default:
                    return 1;
            }
        },
        [](DropdownArgs args) {
            switch (args.index)
            {
                case 0:
                    return Qt::VerPattern;
                case 1:
                default:
                    return Qt::SolidPattern;
            }
        },
        false);
    layout.addColorButton("Line color",
                          QColor(getSettings()->lastMessageColor.getValue()),
                          getSettings()->lastMessageColor);

    layout.addTitle("Emotes");
    layout.addCheckbox("Enable", s.enableEmoteImages);
    layout.addCheckbox("Animate", s.animateEmotes);
    layout.addCheckbox("Animate only when Chatterino is focused",
                       s.animationsWhenFocused);
    layout.addCheckbox(
        "Enable zero-width emotes", s.enableZeroWidthEmotes, false,
        "When disabled, emotes that overlap other emotes, such as BTTV's "
        "cvMask and 7TV's RainTime, will appear as normal emotes.");
    layout.addCheckbox("Enable emote auto-completion by typing :",
                       s.emoteCompletionWithColon);
    layout.addDropdown<float>(
        "Size", {"0.5x", "0.75x", "Default", "1.25x", "1.5x", "2x"},
        s.emoteScale,
        [](auto val) {
            if (val == 1)
                return QString("Default");
            else
                return QString::number(val) + "x";
        },
        [](auto args) {
            return fuzzyToFloat(args.value, 1.f);
        });

    layout.addCheckbox("Remove spaces between emotes",
                       s.removeSpacesBetweenEmotes, false,
                       "When enabled, adjacent emotes will no longer have an "
                       "added space seperating them.");
    layout.addCheckbox("Show unlisted 7TV emotes", s.showUnlistedSevenTVEmotes);
    // TODO: Add a tooltip explaining what an unlisted 7TV emote is
    // but wait until https://github.com/Chatterino/wiki/pull/255 is resolved,
    // as an official description from 7TV devs is best
    s.showUnlistedSevenTVEmotes.connect(
        []() {
            getApp()->twitch->forEachChannelAndSpecialChannels(
                [](const auto &c) {
                    if (c->isTwitchChannel())
                    {
                        auto *channel = dynamic_cast<TwitchChannel *>(c.get());
                        if (channel != nullptr)
                        {
                            channel->refreshSevenTVChannelEmotes(false);
                        }
                    }
                });
        },
        false);
    layout.addDropdown<int>(
        "Show emote & badge thumbnail on hover",
        {"Don't show", "Always show", "Hold shift"}, s.emotesTooltipPreview,
        [](int index) {
            return index;
        },
        [](auto args) {
            return args.index;
        },
        false);
    layout.addDropdown("Emoji style",
                       {
                           "Twitter",
                           "Facebook",
                           "Apple",
                           "Google",
                       },
                       s.emojiSet);
    layout.addCheckbox("Show BTTV global emotes", s.enableBTTVGlobalEmotes);
    layout.addCheckbox("Show BTTV channel emotes", s.enableBTTVChannelEmotes);
    layout.addCheckbox("Enable BTTV live emote updates (requires restart)",
                       s.enableBTTVLiveUpdates);
    layout.addCheckbox("Show FFZ global emotes", s.enableFFZGlobalEmotes);
    layout.addCheckbox("Show FFZ channel emotes", s.enableFFZChannelEmotes);
    layout.addCheckbox("Show 7TV global emotes", s.enableSevenTVGlobalEmotes);
    layout.addCheckbox("Show 7TV channel emotes", s.enableSevenTVChannelEmotes);
    layout.addCheckbox("Enable 7TV live emote updates (requires restart)",
                       s.enableSevenTVEventAPI);

    layout.addTitle("Streamer Mode");
    layout.addDescription(
        "Chatterino can automatically change behavior if it detects that any "
        "streaming software is running.\nSelect which things you want to "
        "change while streaming");

    ComboBox *dankDropdown =
        layout.addDropdown<std::underlying_type<StreamerModeSetting>::type>(
            "Enable Streamer Mode",
            {"Disabled", "Enabled", "Automatic (Detect streaming software)"},
            s.enableStreamerMode,
            [](int value) {
                return value;
            },
            [](DropdownArgs args) {
                return static_cast<StreamerModeSetting>(args.index);
            },
            false);
    dankDropdown->setMinimumWidth(dankDropdown->minimumSizeHint().width() + 30);

    layout.addCheckbox("Hide usercard avatars",
                       s.streamerModeHideUsercardAvatars, false,
                       "Prevent potentially explicit avatars from showing.");
    layout.addCheckbox("Hide link thumbnails", s.streamerModeHideLinkThumbnails,
                       false,
                       "Prevent potentially explicit thumbnails from showing "
                       "when hovering links.");
    layout.addCheckbox(
        "Hide viewer count and stream length while hovering over split header",
        s.streamerModeHideViewerCountAndDuration);
    layout.addCheckbox("Hide moderation actions", s.streamerModeHideModActions,
                       false, "Hide bans & timeouts from appearing in chat.");
    layout.addCheckbox("Mute mention sounds", s.streamerModeMuteMentions, false,
                       "Mute your ping sound from playing.");
    layout.addCheckbox(
        "Suppress Live Notifications", s.streamerModeSuppressLiveNotifications,
        false, "Hide Live notification popups from appearing. (Windows Only)");
    layout.addCheckbox("Suppress Inline Whispers",
                       s.streamerModeSuppressInlineWhispers, false,
                       "Hide whispers sent to you from appearing in chat.");

    layout.addTitle("Link Previews");
    layout.addDescription(
        "Extra information like \"youtube video stats\" or title of webpages "
        "can be loaded for all links if enabled. Optionally you can also show "
        "thumbnails for emotes, videos and more. The information is pulled "
        "from our servers. The Link Previews are loaded through <a "
        "href=\"https://github.com/Chatterino/api\">an API</a> hosted by the "
        "Chatterino developers. These are the API <a "
        "href=\"https://braize.pajlada.com/chatterino/legal/"
        "terms-of-service\">Terms of Services</a> and <a "
        "href=\"https://braize.pajlada.com/chatterino/legal/"
        "privacy-policy\">Privacy Policy</a>.");
    layout.addCheckbox("Enable", s.linkInfoTooltip);
    layout.addDropdown<int>(
        "Also show thumbnails if available",
        {"Off", "Small", "Medium", "Large"}, s.thumbnailSize,
        [](auto val) {
            if (val == 0)
                return QString("Off");
            else if (val == 100)
                return QString("Small");
            else if (val == 200)
                return QString("Medium");
            else if (val == 300)
                return QString("Large");
            else
                return QString::number(val);
        },
        [](auto args) {
            if (args.value == "Small")
                return 100;
            else if (args.value == "Medium")
                return 200;
            else if (args.value == "Large")
                return 300;

            return fuzzyToInt(args.value, 0);
        });
    layout.addDropdown<int>(
        "Show thumbnails of streams", {"Off", "Small", "Medium", "Large"},
        s.thumbnailSizeStream,
        [](auto val) {
            if (val == 0)
                return QString("Off");
            else if (val == 1)
                return QString("Small");
            else if (val == 2)
                return QString("Medium");
            else if (val == 3)
                return QString("Large");
            else
                return QString::number(val);
        },
        [](auto args) {
            if (args.value == "Small")
                return 1;
            else if (args.value == "Medium")
                return 2;
            else if (args.value == "Large")
                return 3;

            return fuzzyToInt(args.value, 0);
        });

    layout.addNavigationSpacing();
    layout.addTitle("Beta");
    if (Version::instance().isSupportedOS())
    {
        layout.addDescription(
            "You can receive updates earlier by ticking the box below. Report "
            "issues <a href='https://chatterino.com/link/issues'>here</a>.");
        layout.addCheckbox("Receive beta updates", s.betaUpdates);
    }
    else
    {
        layout.addDescription(
            "Your operating system is not officially supplied with builds. For "
            "updates, please rebuild Chatterino from sources. Report "
            "issues <a href='https://chatterino.com/link/issues'>here</a>.");
    }

#ifdef Q_OS_WIN
    layout.addTitle("Browser Integration");
    layout.addDescription("The browser extension replaces the default "
                          "Twitch.tv chat with Chatterino.");

    {
        if (auto err = nmIpcError().get())
        {
            layout.addDescription(
                "An error happened during initialization of the "
                "browser extension: " +
                *err);
        }
    }

    layout.addDescription(formatRichNamedLink(
        CHROME_EXTENSION_LINK,
        "Download for Google Chrome and similar browsers."));
    layout.addDescription(
        formatRichNamedLink(FIREFOX_EXTENSION_LINK, "Download for Firefox"));

    layout.addDescription("Chatterino only attaches to known browsers to avoid "
                          "attaching to other windows by accident.");
    layout.addCheckbox(
        "Attach to any browser (may cause issues)",
        s.attachExtensionToAnyProcess, false,
        "Attempt to force the Chatterino Browser Extension to work in certain "
        "browsers that do not work automatically.\ne.g. Librewolf");
#endif

    layout.addTitle("AppData & Cache");

    layout.addSubtitle("Application Data");
    layout.addDescription("All local files like settings and cache files are "
                          "store in this directory.");
    layout.addButton("Open AppData directory", [] {
#ifdef Q_OS_DARWIN
        QDesktopServices::openUrl("file://" + getPaths()->rootAppDataDirectory);
#else
        QDesktopServices::openUrl(getPaths()->rootAppDataDirectory);
#endif
    });

    layout.addSubtitle("Temporary files (Cache)");
    layout.addDescription(
        "Files that are used often (such as emotes) are saved to disk to "
        "reduce bandwidth usage and to speed up loading.");

    auto cachePathLabel = layout.addDescription("placeholder :D");
    getSettings()->cachePath.connect([cachePathLabel](const auto &,
                                                      auto) mutable {
        QString newPath = getPaths()->cacheDirectory();

        QString pathShortened = "Cache saved at <a href=\"file:///" + newPath +
                                "\"><span style=\"color: white;\">" +
                                shortenString(newPath, 50) + "</span></a>";
        cachePathLabel->setText(pathShortened);
        cachePathLabel->setToolTip(newPath);
    });

    // Choose and reset buttons
    {
        auto box = new QHBoxLayout;

        box->addWidget(layout.makeButton("Choose cache path", [this]() {
            getSettings()->cachePath = QFileDialog::getExistingDirectory(this);
        }));
        box->addWidget(layout.makeButton("Reset", []() {
            getSettings()->cachePath = "";
        }));
        box->addWidget(layout.makeButton("Clear Cache", [&layout]() {
            auto reply = QMessageBox::question(
                layout.window(), "Clear cache",
                "Are you sure that you want to clear your cache? Emotes may "
                "take longer to load next time Chatterino is started.",
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                auto cacheDir = QDir(getPaths()->cacheDirectory());
                cacheDir.removeRecursively();
                cacheDir.mkdir(getPaths()->cacheDirectory());
            }
        }));
        box->addStretch(1);

        layout.addLayout(box);
    }

    layout.addTitle("Advanced");

    layout.addSubtitle("Chat title");
    layout.addDescription("In live channels show:");
    layout.addCheckbox("Uptime", s.headerUptime, false,
                       "Show how long the channel has been live");
    layout.addCheckbox("Viewer count", s.headerViewerCount, false,
                       "Show how many users are watching");
    layout.addCheckbox("Category", s.headerGame, false,
                       "Show what Category the stream is listed under");
    layout.addCheckbox("Title", s.headerStreamTitle, false,
                       "Show the stream title");

    layout.addSubtitle("R9K");
    auto toggleLocalr9kSeq = getApp()->hotkeys->getDisplaySequence(
        HotkeyCategory::Window, "toggleLocalR9K");
    QString toggleLocalr9kShortcut =
        "an assigned hotkey (Window -> Toggle local R9K)";
    if (!toggleLocalr9kSeq.isEmpty())
    {
        toggleLocalr9kShortcut = toggleLocalr9kSeq.toString(
            QKeySequence::SequenceFormat::NativeText);
    }
    layout.addDescription("Hide similar messages. Toggle hidden "
                          "messages by pressing " +
                          toggleLocalr9kShortcut + ".");
    layout.addCheckbox("Hide similar messages", s.similarityEnabled);
    //layout.addCheckbox("Gray out matches", s.colorSimilarDisabled);
    layout.addCheckbox("By the same user", s.hideSimilarBySameUser);
    layout.addCheckbox("Hide my own messages", s.hideSimilarMyself);
    layout.addCheckbox("Receive notification sounds from hidden messages",
                       s.shownSimilarTriggerHighlights);
    s.hideSimilar.connect(
        []() {
            getApp()->windows->forceLayoutChannelViews();
        },
        false);
    layout.addDropdown<float>(
        "Similarity threshold", {"0.5", "0.75", "0.9"}, s.similarityPercentage,
        [](auto val) {
            return QString::number(val);
        },
        [](auto args) {
            return fuzzyToFloat(args.value, 0.9f);
        });
    layout.addDropdown<int>(
        "Maximum delay between messages",
        {"5s", "10s", "15s", "30s", "60s", "120s"}, s.hideSimilarMaxDelay,
        [](auto val) {
            return QString::number(val) + "s";
        },
        [](auto args) {
            return fuzzyToInt(args.value, 5);
        });
    layout.addDropdown<int>(
        "Amount of previous messages to check", {"1", "2", "3", "4", "5"},
        s.hideSimilarMaxMessagesToCheck,
        [](auto val) {
            return QString::number(val);
        },
        [](auto args) {
            return fuzzyToInt(args.value, 3);
        });

    layout.addSubtitle("Visible badges");
    layout.addCheckbox("Authority", s.showBadgesGlobalAuthority, false,
                       "e.g. staff, admin");
    layout.addCheckbox("Predictions", s.showBadgesPredictions);
    layout.addCheckbox("Channel", s.showBadgesChannelAuthority, false,
                       "e.g. broadcaster, moderator");
    layout.addCheckbox("Subscriber ", s.showBadgesSubscription);
    layout.addCheckbox("Vanity", s.showBadgesVanity, false,
                       "e.g. prime, bits, sub gifter");
    layout.addCheckbox("Chatterino", s.showBadgesChatterino, false,
                       "e.g. Chatterino Supporter/Contributor/Developer");
    layout.addCheckbox("FrankerFaceZ", s.showBadgesFfz, false,
                       "e.g. Bot, FFZ supporter, FFZ developer");
    layout.addCheckbox("7TV", s.showBadgesSevenTV, false,
                       "Badges for 7TV admins, developers, and supporters");
    layout.addSeperator();
    layout.addCheckbox("Use custom FrankerFaceZ moderator badges",
                       s.useCustomFfzModeratorBadges);
    layout.addCheckbox("Use custom FrankerFaceZ VIP badges",
                       s.useCustomFfzVipBadges);

    layout.addSubtitle("Miscellaneous");

    if (supportsIncognitoLinks())
    {
        layout.addCheckbox("Open links in incognito/private mode",
                           s.openLinksIncognito);
    }

    layout.addCheckbox(
        "Restart on crash", s.restartOnCrash, false,
        "When possible, restart Chatterino if the program crashes");

#if defined(Q_OS_LINUX) && !defined(NO_QTKEYCHAIN)
    if (!getPaths()->isPortable())
    {
        layout.addCheckbox(
            "Use libsecret/KWallet/Gnome keychain to secure passwords",
            s.useKeyring);
    }
#endif

    layout.addCheckbox(
        "Show moderation messages", s.hideModerationActions, true,
        "Show messages for timeouts, bans, and other moderator actions.");
    layout.addCheckbox("Show deletions of single messages",
                       s.hideDeletionActions, true,
                       "Show when a single message is deleted.\ne.g. A message "
                       "from TreuKS was deleted: abc");
    layout.addCheckbox(
        "Colorize users without color set (gray names)", s.colorizeNicknames,
        false,
        "Grant a random color to users who never set a color for themselves");
    layout.addCheckbox("Mention users with a comma", s.mentionUsersWithComma,
                       false,
                       "When using tab-completon, if the username is at the "
                       "start of the message, include a comma at the end of "
                       "the name.\ne.g. pajl -> pajlada,");
    layout.addCheckbox(
        "Show joined users (< 1000 chatters)", s.showJoins, false,
        "Show a Twitch system message stating what users have joined the chat, "
        "only available when the chat has less than 1000 users");
    layout.addCheckbox(
        "Show parted users (< 1000 chatters)", s.showParts, false,
        "Show a Twitch system message stating what users have left the chat, "
        "only available when chat has less than 1000 users");
    layout.addCheckbox("Automatically close user popup when it loses focus",
                       s.autoCloseUserPopup);
    layout.addCheckbox(
        "Automatically close reply thread popup when it loses focus",
        s.autoCloseThreadPopup);
    layout.addCheckbox("Lowercase domains (anti-phishing)", s.lowercaseDomains,
                       false,
                       "Make all clickable links lowercase to deter "
                       "phishing attempts.");
    layout.addCheckbox("Bold @usernames", s.boldUsernames, false,
                       "Bold @mentions to make them more noticable.");
    layout.addCheckbox("Color @usernames", s.colorUsernames, false,
                       "If Chatterino has seen a user, highlight @mention's of "
                       "them with their Twitch color.");
    layout.addCheckbox("Try to find usernames without @ prefix",
                       s.findAllUsernames, false,
                       "Find mentions of users in chat without the @ prefix.");
    layout.addCheckbox("Show username autocompletion popup menu",
                       s.showUsernameCompletionMenu);
    const QStringList usernameDisplayModes = {"Username", "Localized name",
                                              "Username and localized name"};

    ComboBox *nameDropdown =
        layout.addDropdown<std::underlying_type<UsernameDisplayMode>::type>(
            "Username style", usernameDisplayModes, s.usernameDisplayMode,
            [usernameDisplayModes](auto val) {
                return usernameDisplayModes.at(val - 1);
                // UsernameDisplayMode enum indexes from 1
            },
            [](auto args) {
                return args.index + 1;
            },
            false,
            "Customizes how you see Asian Language names.\nUsing an option "
            "that includes \"localized\" will display the username in it's "
            "respective Asian language.\ne.g. "
            "Username and localized: testaccount_420(테스트계정420)\n"
            "Username: testaccount_420\n"
            "Localized name: 테스트계정420");
    nameDropdown->setMinimumWidth(nameDropdown->minimumSizeHint().width());

    layout.addDropdown<float>(
        "Username font weight", {"50", "Default", "75", "100"}, s.boldScale,
        [](auto val) {
            if (val == 63)
                return QString("Default");
            else
                return QString::number(val);
        },
        [](auto args) {
            return fuzzyToFloat(args.value, 63.f);
        });
    layout.addCheckbox("Double click to open links and other elements in chat",
                       s.linksDoubleClickOnly, false,
                       "When enabled, opening links/usercards requires "
                       "double-clicking.\nUseful making sure you don't "
                       "accidentally click on suspicious links.");
    layout.addCheckbox(
        "Unshorten links", s.unshortLinks, false,
        "When enabled, \"right-click + copy link\" will copy the unshortened "
        "version of the link.\ne.g. https://bit.ly/mrfors -> "
        "https://forsen.tv/");

    layout.addCheckbox(
        "Only search for emote autocompletion at the start of emote names",
        s.prefixOnlyEmoteCompletion, false,
        "When disabled, emote tab-completion will complete based on any part "
        "of the name."
        "\ne.g. sheffy -> DatSheffy");
    layout.addCheckbox(
        "Only search for username autocompletion with an @",
        s.userCompletionOnlyWithAt, false,
        "When enabled, username tab-completion will only complete when using @"
        "\ne.g. pajl -> pajl | @pajl -> @pajlada");

    layout.addCheckbox("Show Twitch whispers inline", s.inlineWhispers, false,
                       "Show whispers as messages in all splits instead "
                       "of just /whispers.");
    layout.addCheckbox(
        "Highlight received inline whispers", s.highlightInlineWhispers, false,
        "Highlight the whispers shown in all splits.\nIf \"Show Twitch "
        "whispers inline\" is disabled, this setting will do nothing.");
    layout.addCheckbox("Load message history on connect",
                       s.loadTwitchMessageHistoryOnConnect);
    // TODO: Change phrasing to use better english once we can tag settings, right now it's kept as history instead of historical so that the setting shows up when the user searches for history
    layout.addIntInput("Max number of history messages to load on connect",
                       s.twitchMessageHistoryLimit, 10, 800, 10);

    layout.addIntInput("Split message scrollback limit (requires restart)",
                       s.scrollbackSplitLimit, 100, 100000, 100);
    layout.addIntInput("Usercard scrollback limit (requires restart)",
                       s.scrollbackUsercardLimit, 100, 100000, 100);

    layout.addCheckbox("Enable experimental IRC support (requires restart)",
                       s.enableExperimentalIrc, false,
                       "When enabled, attempting to join a channel will "
                       "include an \"IRC (Beta)\" tab allowing the user to "
                       "connect to an IRC server outside of Twitch ");
    layout.addCheckbox("Show unhandled IRC messages",
                       s.showUnhandledIrcMessages);
    layout.addDropdown<int>(
        "Stack timeouts", {"Stack", "Stack until timeout", "Don't stack"},
        s.timeoutStackStyle,
        [](int index) {
            return index;
        },
        [](auto args) {
            return args.index;
        },
        false, "Combine consecutive timeout messages into a single message.");
    layout.addCheckbox("Combine multiple bit tips into one", s.stackBits, false,
                       "Combine consecutive cheermotes (sent in a single "
                       "message) into one cheermote.");
    layout.addCheckbox(
        "Messages in /mentions highlights tab", s.highlightMentions, false,
        // update this tooltip if https://github.com/Chatterino/chatterino2/pull/1557 is ever merged
        "When disabled, the /mentions tab will not highlight in "
        "red when you are mentioned.");
    layout.addCheckbox(
        "Strip leading mention in replies", s.stripReplyMention, false,
        "When disabled, messages sent in reply threads will include the "
        "@mention for the related thread. If the reply context is hidden, "
        "these mentions will never be stripped.");

    // Helix timegate settings
    auto helixTimegateGetValue = [](auto val) {
        switch (val)
        {
            case HelixTimegateOverride::Timegate:
                return "Timegate";
            case HelixTimegateOverride::AlwaysUseIRC:
                return "Always use IRC";
            case HelixTimegateOverride::AlwaysUseHelix:
                return "Always use Helix";
            default:
                return "Timegate";
        }
    };

    auto helixTimegateSetValue = [](auto args) {
        const auto &v = args.value;
        if (v == "Timegate")
        {
            return HelixTimegateOverride::Timegate;
        }
        if (v == "Always use IRC")
        {
            return HelixTimegateOverride::AlwaysUseIRC;
        }
        if (v == "Always use Helix")
        {
            return HelixTimegateOverride::AlwaysUseHelix;
        }

        qCDebug(chatterinoSettings) << "Unknown Helix timegate override value"
                                    << v << ", using default value Timegate";
        return HelixTimegateOverride::Timegate;
    };

    auto *helixTimegateRaid =
        layout.addDropdown<std::underlying_type<HelixTimegateOverride>::type>(
            "Helix timegate /raid behaviour",
            {"Timegate", "Always use IRC", "Always use Helix"},
            s.helixTimegateRaid,
            helixTimegateGetValue,  //
            helixTimegateSetValue,  //
            false);
    helixTimegateRaid->setMinimumWidth(
        helixTimegateRaid->minimumSizeHint().width());

    auto *helixTimegateWhisper =
        layout.addDropdown<std::underlying_type<HelixTimegateOverride>::type>(
            "Helix timegate /w behaviour",
            {"Timegate", "Always use IRC", "Always use Helix"},
            s.helixTimegateWhisper,
            helixTimegateGetValue,  //
            helixTimegateSetValue,  //
            false);
    helixTimegateWhisper->setMinimumWidth(
        helixTimegateWhisper->minimumSizeHint().width());

    auto *helixTimegateVIPs =
        layout.addDropdown<std::underlying_type<HelixTimegateOverride>::type>(
            "Helix timegate /vips behaviour",
            {"Timegate", "Always use IRC", "Always use Helix"},
            s.helixTimegateVIPs,
            helixTimegateGetValue,  //
            helixTimegateSetValue,  //
            false);
    helixTimegateVIPs->setMinimumWidth(
        helixTimegateVIPs->minimumSizeHint().width());

    auto *helixTimegateCommercial =
        layout.addDropdown<std::underlying_type<HelixTimegateOverride>::type>(
            "Helix timegate /commercial behaviour",
            {"Timegate", "Always use IRC", "Always use Helix"},
            s.helixTimegateCommercial,
            helixTimegateGetValue,  //
            helixTimegateSetValue,  //
            false);
    helixTimegateCommercial->setMinimumWidth(
        helixTimegateCommercial->minimumSizeHint().width());

    auto *helixTimegateModerators =
        layout.addDropdown<std::underlying_type<HelixTimegateOverride>::type>(
            "Helix timegate /mods behaviour",
            {"Timegate", "Always use IRC", "Always use Helix"},
            s.helixTimegateModerators,
            helixTimegateGetValue,  //
            helixTimegateSetValue,  //
            false);
    helixTimegateModerators->setMinimumWidth(
        helixTimegateModerators->minimumSizeHint().width());

    layout.addStretch();

    // invisible element for width
    auto inv = new BaseWidget(this);
    //    inv->setScaleIndependantWidth(600);
    layout.addWidget(inv);
}

void GeneralPage::initExtra()
{
    /// update cache path
    if (this->cachePath_)
    {
        getSettings()->cachePath.connect(
            [cachePath = this->cachePath_](const auto &, auto) mutable {
                QString newPath = getPaths()->cacheDirectory();

                QString pathShortened = "Current location: <a href=\"file:///" +
                                        newPath + "\">" +
                                        shortenString(newPath, 50) + "</a>";

                cachePath->setText(pathShortened);
                cachePath->setToolTip(newPath);
            });
    }
}

QString GeneralPage::getFont(const DropdownArgs &args) const
{
    if (args.combobox->currentIndex() == args.combobox->count() - 1)
    {
        args.combobox->setCurrentIndex(0);
        args.combobox->setEditText("Choosing...");
        QFontDialog dialog(getApp()->fonts->getFont(FontStyle::ChatMedium, 1.));

        auto ok = bool();
        auto font = dialog.getFont(&ok, this->window());

        if (ok)
            return font.family();
        else
            return args.combobox->itemText(0);
    }
    return args.value;
}

}  // namespace chatterino
