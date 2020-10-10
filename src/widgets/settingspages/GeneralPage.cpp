#include "GeneralPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "Application.hpp"
#include "common/Version.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FuzzyConvert.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/ColorButton.hpp"
#include "widgets/helper/ExpandableLayout.hpp"
#include "widgets/helper/Line.hpp"

#define CHROME_EXTENSION_LINK                                           \
    "https://chrome.google.com/webstore/detail/chatterino-native-host/" \
    "glknmaideaikkmemifbfkhnomoknepka"
#define FIREFOX_EXTENSION_LINK \
    "https://addons.mozilla.org/en-US/firefox/addon/chatterino-native-host/"

// define to highlight sections in editor
#define addTitle addTitle

#ifdef Q_OS_WIN
#    define META_KEY "Windows"
#else
#    define META_KEY "Meta"
#endif

namespace chatterino {
namespace {
    void addKeyboardModifierSetting(SettingsLayout &layout,
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

TitleLabel *SettingsLayout::addTitle(const QString &title)
{
    // space
    if (!this->groups_.empty())
        this->addWidget(this->groups_.back().space = new Space);

    // title
    auto label = new TitleLabel(title + ":");
    this->addWidget(label);

    // groups
    this->groups_.push_back(Group{title, label, nullptr, {}});

    return label;
}

QCheckBox *SettingsLayout::addCheckbox(const QString &text,
                                       BoolSetting &setting, bool inverse)
{
    auto check = new QCheckBox(text);

    // update when setting changes
    setting.connect(
        [inverse, check](const bool &value, auto) {
            check->setChecked(inverse ^ value);
        },
        this->managedConnections_);

    // update setting on toggle
    QObject::connect(
        check, &QCheckBox::toggled, this,
        [&setting, inverse](bool state) { setting = inverse ^ state; });

    this->addWidget(check);

    // groups
    this->groups_.back().widgets.push_back({check, {text}});

    return check;
}

ComboBox *SettingsLayout::addDropdown(const QString &text,
                                      const QStringList &list)
{
    auto layout = new QHBoxLayout;
    auto combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);
    combo->addItems(list);

    auto label = new QLabel(text + ":");
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(combo);

    this->addLayout(layout);

    // groups
    this->groups_.back().widgets.push_back({combo, {text}});
    this->groups_.back().widgets.push_back({label, {text}});

    return combo;
}

ComboBox *SettingsLayout::addDropdown(
    const QString &text, const QStringList &items,
    pajlada::Settings::Setting<QString> &setting, bool editable)
{
    auto combo = this->addDropdown(text, items);

    if (editable)
        combo->setEditable(true);

    // update when setting changes
    setting.connect(
        [combo](const QString &value, auto) { combo->setCurrentText(value); },
        this->managedConnections_);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [&setting](const QString &newValue) {
                         setting = newValue;
                         getApp()->windows->forceLayoutChannelViews();
                     });

    return combo;
}

ColorButton *SettingsLayout::addColorButton(
    const QString &text, const QColor &color,
    pajlada::Settings::Setting<QString> &setting)
{
    auto colorButton = new ColorButton(color);
    auto layout = new QHBoxLayout();
    auto label = new QLabel(text + ":");
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(colorButton);
    this->addLayout(layout);
    QObject::connect(
        colorButton, &ColorButton::clicked, [&setting, colorButton]() {
            auto dialog = new ColorPickerDialog(QColor(setting));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->closed.connect([&setting, colorButton, &dialog] {
                QColor selected = dialog->selectedColor();

                if (selected.isValid())
                {
                    setting = selected.name(QColor::HexArgb);
                    colorButton->setColor(selected);
                }
            });
        });

    return colorButton;
}

DescriptionLabel *SettingsLayout::addDescription(const QString &text)
{
    auto label = new DescriptionLabel(text);

    label->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                   Qt::LinksAccessibleByKeyboard);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);

    this->addWidget(label);

    // groups
    this->groups_.back().widgets.push_back({label, {text}});

    return label;
}

void SettingsLayout::addSeperator()
{
    this->addWidget(new Line(false));
}

bool SettingsLayout::filterElements(const QString &query)
{
    bool any{};

    for (auto &&group : this->groups_)
    {
        // if a description in a group matches `query` then show the entire group
        bool descriptionMatches{};
        for (auto &&widget : group.widgets)
        {
            if (auto x = dynamic_cast<DescriptionLabel *>(widget.element); x)
            {
                if (x->text().contains(query, Qt::CaseInsensitive))
                {
                    descriptionMatches = true;
                    break;
                }
            }
        }

        // if group name matches then all should be visible
        if (group.name.contains(query, Qt::CaseInsensitive) ||
            descriptionMatches)
        {
            for (auto &&widget : group.widgets)
                widget.element->show();
            group.title->show();
            any = true;
        }
        // check if any match
        else
        {
            auto groupAny = false;

            for (auto &&widget : group.widgets)
            {
                for (auto &&keyword : widget.keywords)
                {
                    if (keyword.contains(query, Qt::CaseInsensitive))
                    {
                        widget.element->show();
                        groupAny = true;
                    }
                    else
                    {
                        widget.element->hide();
                    }
                }
            }

            if (group.space)
                group.space->setVisible(groupAny);
            group.title->setVisible(groupAny);
            any |= groupAny;
        }
    }

    return any;
}

GeneralPage::GeneralPage()
{
    auto y = new QVBoxLayout;
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    y->addWidget(scroll);
    auto x = new QHBoxLayout;
    auto layout = new SettingsLayout;
    this->settingsLayout_ = layout;
    x->addLayout(layout, 0);
    x->addStretch(1);
    auto z = new QFrame;
    z->setLayout(x);
    scroll->setWidget(z);
    this->setLayout(y);

    this->initLayout(*layout);

    layout->addStretch(1);

    this->initExtra();
}

bool GeneralPage::filterElements(const QString &query)
{
    if (this->settingsLayout_)
        return this->settingsLayout_->filterElements(query) || query.isEmpty();
    else
        return false;
}

void GeneralPage::initLayout(SettingsLayout &layout)
{
    auto &s = *getSettings();

    layout.addTitle("Interface");
    layout.addDropdown("Theme", {"White", "Light", "Dark", "Black"},
                       getApp()->themes->themeName);
    layout.addDropdown<QString>(
        "Font", {"Segoe UI", "Arial", "Choose..."},
        getApp()->fonts->chatFontFamily, [](auto val) { return val; },
        [this](auto args) { return this->getFont(args); });
    layout.addDropdown<int>(
        "Font size", {"9pt", "10pt", "12pt", "14pt", "16pt", "20pt"},
        getApp()->fonts->chatFontSize,
        [](auto val) { return QString::number(val) + "pt"; },
        [](auto args) { return fuzzyToInt(args.value, 10); });
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
        [](auto args) { return fuzzyToFloat(args.value, 1.f); });
    layout.addDropdown<int>(
        "Tab layout", {"Horizontal", "Vertical"}, s.tabDirection,
        [](auto val) {
            switch (val)
            {
                case NotebookTabDirection::Horizontal:
                    return "Horizontal";
                case NotebookTabDirection::Vertical:
                    return "Vertical";
            }

            return "";
        },
        [](auto args) {
            if (args.value == "Vertical")
            {
                return NotebookTabDirection::Vertical;
            }
            else
            {
                // default to horizontal
                return NotebookTabDirection::Horizontal;
            }
        });

    layout.addCheckbox("Always on top", s.windowTopMost);
#ifdef USEWINSDK
    layout.addCheckbox("Start with Windows", s.autorun);
#endif

    layout.addTitle("Chat");

    layout.addDropdown<float>(
        "Mousewheel scroll speed", {"0.5x", "0.75x", "Default", "1.5x", "2x"},
        s.mouseScrollMultiplier,
        [](auto val) {
            if (val == 1)
                return QString("Default");
            else
                return QString::number(val) + "x";
        },
        [](auto args) { return fuzzyToFloat(args.value, 1.f); });
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
    layout.addCheckbox("Smooth scrolling", s.enableSmoothScrolling);
    layout.addCheckbox("Smooth scrolling on new messages",
                       s.enableSmoothScrollingNewMessages);

    layout.addTitle("Messages");
    layout.addCheckbox("Separate with lines", s.separateMessages);
    layout.addCheckbox("Alternate background color", s.alternateMessages);
    layout.addCheckbox("Show deleted messages", s.hideModerated, true);
    layout.addCheckbox("Show last message line", s.showLastMessageIndicator);
    layout.addDropdown<std::underlying_type<Qt::BrushStyle>::type>(
        "Last message line style", {"Dotted", "Solid"}, s.lastMessagePattern,
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
    layout.addColorButton("Last message line color",
                          QColor(getSettings()->lastMessageColor.getValue()),
                          getSettings()->lastMessageColor);
    layout.addCheckbox("Highlight messages redeemed with Channel Points",
                       s.enableRedeemedHighlight);
    layout.addDropdown<QString>(
        "Timestamp format (a = am/pm)",
        {"Disable", "h:mm", "hh:mm", "h:mm a", "hh:mm a", "h:mm:ss", "hh:mm:ss",
         "h:mm:ss a", "hh:mm:ss a"},
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
        });
    layout.addDropdown<int>(
        "Limit message height",
        {"Never", "2 lines", "3 lines", "4 lines", "5 lines"},
        s.collpseMessagesMinLines,
        [](auto val) {
            return val ? QString::number(val) + " lines" : QString("Never");
        },
        [](auto args) { return fuzzyToInt(args.value, 0); });

    layout.addTitle("Link Information");
    layout.addDescription(
        "Extra information like \"youtube video stats\" or title of webpages "
        "can be loaded for all links if enabled. Optionally you can also show "
        "thumbnails for emotes, videos and more. The information is pulled "
        "from our servers.");
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

    layout.addTitle("Emotes");
    layout.addCheckbox("Enable", s.enableEmoteImages);
    layout.addCheckbox("Animate", s.animateEmotes);
    layout.addCheckbox("Animate only when Chatterino is focused",
                       s.animationsWhenFocused);
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
        [](auto args) { return fuzzyToFloat(args.value, 1.f); });

    layout.addDropdown<int>(
        "Show info on hover", {"Don't show", "Always show", "Hold shift"},
        s.emotesTooltipPreview, [](int index) { return index; },
        [](auto args) { return args.index; }, false);
    layout.addDropdown("Emoji style",
                       {"EmojiOne 2", "EmojiOne 3", "Twitter", "Facebook",
                        "Apple", "Google", "Messenger"},
                       s.emojiSet);

    layout.addTitle("R9K");
    layout.addDescription(
        "Hide similar messages by the same user. Toggle hidden "
        "messages by pressing Ctrl+H.");
    layout.addCheckbox("Hide similar messages", s.similarityEnabled);
    //layout.addCheckbox("Gray out matches", s.colorSimilarDisabled);
    layout.addCheckbox("Hide my own messages", s.hideSimilarMyself);
    layout.addCheckbox("Receive notification sounds from hidden messages",
                       s.shownSimilarTriggerHighlights);
    s.hideSimilar.connect(
        []() { getApp()->windows->forceLayoutChannelViews(); }, false);
    layout.addDropdown<float>(
        "Similarity threshold", {"0.5", "0.75", "0.9"}, s.similarityPercentage,
        [](auto val) { return QString::number(val); },
        [](auto args) { return fuzzyToFloat(args.value, 0.9f); });
    layout.addDropdown<int>(
        "Maximum delay between messages",
        {"5s", "10s", "15s", "30s", "60s", "120s"}, s.hideSimilarMaxDelay,
        [](auto val) { return QString::number(val) + "s"; },
        [](auto args) { return fuzzyToInt(args.value, 5); });
    layout.addDropdown<int>(
        "Amount of previous messages to check", {"1", "2", "3", "4", "5"},
        s.hideSimilarMaxMessagesToCheck,
        [](auto val) { return QString::number(val); },
        [](auto args) { return fuzzyToInt(args.value, 3); });

    layout.addTitle("Visible badges");
    layout.addCheckbox("Authority (staff, admin)",
                       getSettings()->showBadgesGlobalAuthority);
    layout.addCheckbox("Channel (broadcaster, moderator)",
                       getSettings()->showBadgesChannelAuthority);
    layout.addCheckbox("Subscriber ", getSettings()->showBadgesSubscription);
    layout.addCheckbox("Vanity (prime, bits, subgifter)",
                       getSettings()->showBadgesVanity);
    layout.addCheckbox("Chatterino", getSettings()->showBadgesChatterino);

    layout.addTitle("Chat title");
    layout.addDescription("In live channels show:");
    layout.addCheckbox("Uptime", s.headerUptime);
    layout.addCheckbox("Viewer count", s.headerViewerCount);
    layout.addCheckbox("Category", s.headerGame);
    layout.addCheckbox("Title", s.headerStreamTitle);

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
            "updates, please rebuild chatterino from sources. Report "
            "issues <a href='https://chatterino.com/link/issues'>here</a>.");
    }

#ifdef Q_OS_WIN
    layout.addTitle("Browser Integration");
    layout.addDescription("The browser extension replaces the default "
                          "Twitch.tv chat with chatterino.");

    layout.addDescription(formatRichNamedLink(
        CHROME_EXTENSION_LINK,
        "Download for Google Chrome and similar browsers."));
    layout.addDescription(
        formatRichNamedLink(FIREFOX_EXTENSION_LINK, "Download for Firefox"));

    layout.addDescription("Chatterino only attaches to known browsers to avoid "
                          "attaching to other windows by accident.");
    layout.addCheckbox("Attach to any browser (may cause issues).",
                       s.attachExtensionToAnyProcess);
#endif

    layout.addTitle("Cache");
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
        box->addWidget(layout.makeButton(
            "Reset", []() { getSettings()->cachePath = ""; }));
        box->addStretch(1);

        layout.addLayout(box);
    }

    layout.addTitle("AppData");
    layout.addDescription("All local files like settings and cache files are "
                          "store in this directory.");
    layout.addButton("Open AppData directory", [] {
        QDesktopServices::openUrl(getPaths()->rootAppDataDirectory);
    });

    layout.addTitle("Miscellaneous");

    if (supportsIncognitoLinks())
    {
        layout.addCheckbox("Open links in incognito/private mode",
                           s.openLinksIncognito);
    }

    layout.addCheckbox("Restart on crash", s.restartOnCrash);

#ifdef Q_OS_LINUX
    if (!getPaths()->isPortable())
    {
        layout.addCheckbox(
            "Use libsecret/KWallet/Gnome keychain to secure passwords",
            s.useKeyring);
    }
#endif

    layout.addCheckbox("Show moderation messages", s.hideModerationActions,
                       true);
    layout.addCheckbox("Colorize users without color set (gray names)",
                       s.colorizeNicknames);
    layout.addCheckbox("Mention users with a comma (User,)",
                       s.mentionUsersWithComma);
    layout.addCheckbox("Show joined users (< 1000 chatters)", s.showJoins);
    layout.addCheckbox("Show parted users (< 1000 chatters)", s.showParts);
    layout.addCheckbox("Automatically close user popup when it loses focus",
                       s.autoCloseUserPopup);
    layout.addCheckbox("Lowercase domains (anti-phishing)", s.lowercaseDomains);
    layout.addCheckbox("Bold @usernames", s.boldUsernames);
    layout.addCheckbox("Try to find usernames without @ prefix",
                       s.findAllUsernames);
    layout.addDropdown<float>(
        "Username font weight", {"50", "Default", "75", "100"}, s.boldScale,
        [](auto val) {
            if (val == 63)
                return QString("Default");
            else
                return QString::number(val);
        },
        [](auto args) { return fuzzyToFloat(args.value, 63.f); });
    layout.addCheckbox("Double click to open links and other elements in chat",
                       s.linksDoubleClickOnly);
    layout.addCheckbox("Unshorten links", s.unshortLinks);

    layout.addCheckbox(
        "Only search for emote autocompletion at the start of emote names",
        s.prefixOnlyEmoteCompletion);
    layout.addCheckbox("Only search for username autocompletion with an @",
                       s.userCompletionOnlyWithAt);

    layout.addCheckbox("Show twitch whispers inline", s.inlineWhispers);
    layout.addCheckbox("Highlight received inline whispers",
                       s.highlightInlineWhispers);
    layout.addCheckbox("Load message history on connect",
                       s.loadTwitchMessageHistoryOnConnect);

    layout.addCheckbox("Enable experimental IRC support (requires restart)",
                       s.enableExperimentalIrc);
    layout.addCheckbox("Show unhandled IRC messages",
                       s.showUnhandledIrcMessages);
    layout.addCheckbox(
        "Hide viewercount and stream length while hovering the split",
        s.hideViewerCountAndDuration);
    layout.addDropdown<int>(
        "Stack timeouts", {"Stack", "Stack until timeout", "Don't stack"},
        s.timeoutStackStyle, [](int index) { return index; },
        [](auto args) { return args.index; }, false);
    layout.addCheckbox("Combine multiple bit tips into one", s.stackBits);
    layout.addCheckbox("Ask for confirmation when uploading an image",
                       s.askOnImageUpload);

    // Advanced Settings setup
    {
        auto *const advancedSettings =
            new ExpandableLayout("Advanced Settings", this);
        auto *const advancedSettingsLayout =
            this->buildAdvancedSettingsLayout();
        advancedSettings->setContent(advancedSettingsLayout);
        layout.addWidget(advancedSettings);
    }

    // invisible element for width
    auto inv = new BaseWidget(this);
    inv->setScaleIndependantWidth(500);
    layout.addWidget(inv);
}

QLayout *GeneralPage::buildAdvancedSettingsLayout()
{
    auto &s = *getSettings();
    auto layout = new SettingsLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    // "Interface" section
    {
        layout->addTitle("Interface");
        layout->addCheckbox("Show tab close button", s.showTabCloseButton);

        if (!BaseWindow::supportsCustomWindowFrame())
        {
            layout->addCheckbox("Show preferences button (Ctrl+P to show)",
                                s.hidePreferencesButton, true);
            layout->addCheckbox("Show user button", s.hideUserButton, true);
        }

        layout->addCheckbox("Show which channels are live in tabs",
                            s.showTabLive);
    }

    // "Chat" section
    {
        layout->addTitle("Chat");
        layout->addCheckbox("Show input when it's empty", s.showEmptyInput);
        layout->addCheckbox("Show message length while typing",
                            s.showMessageLength);
        layout->addCheckbox("Allow sending duplicate messages",
                            s.allowDuplicateMessages);
    }

    return layout;
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

        dialog.setWindowFlag(Qt::WindowStaysOnTopHint);

        auto ok = bool();
        auto font = dialog.getFont(&ok);

        if (ok)
            return font.family();
        else
            return args.combobox->itemText(0);
    }
    return args.value;
}

}  // namespace chatterino
