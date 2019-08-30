#include "GeneralPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FuzzyConvert.hpp"
#include "util/Helpers.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/Line.hpp"

#define CHROME_EXTENSION_LINK                                           \
    "https://chrome.google.com/webstore/detail/chatterino-native-host/" \
    "glknmaideaikkmemifbfkhnomoknepka"
#define FIREFOX_EXTENSION_LINK \
    "https://addons.mozilla.org/en-US/firefox/addon/chatterino-native-host/"

namespace chatterino {
namespace {
    QPushButton *makeOpenSettingDirButton()
    {
        auto button = new QPushButton("Open settings directory");
        QObject::connect(button, &QPushButton::clicked, [] {
            QDesktopServices::openUrl(getPaths()->rootAppDataDirectory);
        });
        return button;
    }
}  // namespace

TitleLabel *SettingsLayout::addTitle(const QString &title)
{
    auto label = new TitleLabel(title + ":");

    if (this->count() != 0)
        this->addSpacing(16);

    this->addWidget(label);
    return label;
}

TitleLabel2 *SettingsLayout::addTitle2(const QString &title)
{
    auto label = new TitleLabel2(title + ":");

    this->addSpacing(16);

    this->addWidget(label);
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
    return check;
}

ComboBox *SettingsLayout::addDropdown(const QString &text,
                                      const QStringList &list)
{
    auto layout = new QHBoxLayout;
    auto combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);
    combo->addItems(list);

    layout->addWidget(new QLabel(text + ":"));
    layout->addStretch(1);
    layout->addWidget(combo);

    this->addLayout(layout);
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

DescriptionLabel *SettingsLayout::addDescription(const QString &text)
{
    auto label = new DescriptionLabel(text);

    label->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                   Qt::LinksAccessibleByKeyboard);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);

    this->addWidget(label);

    return label;
}

void SettingsLayout::addSeperator()
{
    this->addWidget(new Line(false));
}

GeneralPage::GeneralPage()
    : SettingsPage("General", ":/settings/about.svg")
{
    auto y = new QVBoxLayout;
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    y->addWidget(scroll);
    auto x = new QHBoxLayout;
    auto layout = new SettingsLayout;
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
        "UI Scale",
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
    layout.addCheckbox("Show tab close button", s.showTabCloseButton);
    layout.addCheckbox("Always on top", s.windowTopMost);
#ifdef USEWINSDK
    layout.addCheckbox("Start with Windows", s.autorun);
#endif

    layout.addTitle("Chat");

    layout.addDropdown<float>(
        "Mouse scroll speed", {"0.5x", "0.75x", "Default", "1.5x", "2x"},
        s.mouseScrollMultiplier,
        [](auto val) {
            if (val == 1)
                return QString("Default");
            else
                return QString::number(val) + "x";
        },
        [](auto args) { return fuzzyToFloat(args.value, 1.f); });
    layout.addCheckbox("Smooth scrolling", s.enableSmoothScrolling);
    layout.addCheckbox("Smooth scrolling on new messages",
                       s.enableSmoothScrollingNewMessages);
    layout.addCheckbox("Pause on hover", s.pauseChatOnHover);
    layout.addCheckbox("Show input when it's empty", s.showEmptyInput);
    layout.addCheckbox("Show message length while typing", s.showMessageLength);
    if (!BaseWindow::supportsCustomWindowFrame())
    {
        layout.addCheckbox("Show preferences button (ctrl+p to show)",
                           s.hidePreferencesButton, true);
        layout.addCheckbox("Show user button", s.hideUserButton, true);
    }

    layout.addTitle("Messages");
    layout.addCheckbox("Seperate with lines", s.separateMessages);
    layout.addCheckbox("Alternate background color", s.alternateMessages);
    // layout.addCheckbox("Mark last message you read");
    // layout.addDropdown("Last read message style", {"Default"});
    layout.addCheckbox("Show deleted messages", s.hideModerated, true);
    layout.addCheckbox("Show moderation messages", s.hideModerationActions,
                       true);
    layout.addCheckbox("Random username color for users who never set a color",
                       s.colorizeNicknames);
    layout.addDropdown<QString>(
        "Timestamps", {"Disable", "h:mm", "hh:mm", "h:mm a", "hh:mm a"},
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
        "Collapse messages",
        {"Never", "After 2 lines", "After 3 lines", "After 4 lines",
         "After 5 lines"},
        s.collpseMessagesMinLines,
        [](auto val) {
            return val ? QString("After ") + QString::number(val) + " lines"
                       : QString("Never");
        },
        [](auto args) { return fuzzyToInt(args.value, 0); });
    layout.addDropdown<int>(
        "Stack timeouts", {"Stack", "Stack until timeout", "Don't stack"},
        s.timeoutStackStyle, [](int index) { return index; },
        [](auto args) { return args.index; }, false);

    layout.addTitle("Emotes");
    layout.addCheckbox("Enable", s.enableEmoteImages);
    layout.addCheckbox("Animate", s.animateEmotes);
    layout.addCheckbox("Animate only when Chatterino is focused",
                       s.animationsWhenFocused);
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
    layout.addDropdown("Emoji set",
                       {"EmojiOne 2", "EmojiOne 3", "Twitter", "Facebook",
                        "Apple", "Google", "Messenger"},
                       s.emojiSet);

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
    layout.addWidget(new QLabel("In live channels show:"));
    layout.addCheckbox("Uptime", s.headerUptime);
    layout.addCheckbox("Viewer count", s.headerViewerCount);
    layout.addCheckbox("Category", s.headerGame);
    layout.addCheckbox("Title", s.headerStreamTitle);

    layout.addTitle("Miscellaneous");

    //layout.addWidget(makeOpenSettingDirButton());
    layout.addCheckbox("Mention users with a comma (User,)",
                       s.mentionUsersWithComma);
    layout.addCheckbox("Show joined users (< 1000 chatters)", s.showJoins);
    layout.addCheckbox("Show parted users (< 1000 chatters)", s.showParts);
    layout.addCheckbox("Lowercase domains (anti-phisching)",
                       s.lowercaseDomains);
    layout.addCheckbox("Bold @usernames", s.boldUsernames);
    layout.addDropdown<float>(
        "Username font weight", {"50", "Default", "75", "100"}, s.boldScale,
        [](auto val) {
            if (val == 63)
                return QString("Default");
            else
                return QString::number(val);
        },
        [](auto args) { return fuzzyToFloat(args.value, 63.f); });
    layout.addCheckbox("Show link info when hovering", s.linkInfoTooltip);
    layout.addCheckbox("Double click to open links and other elements in chat",
                       s.linksDoubleClickOnly);
    layout.addCheckbox("Unshorten links", s.unshortLinks);
    layout.addCheckbox("Show live indicator in tabs", s.showTabLive);
    layout.addDropdown<int>("Show emote preview in tooltip on hover",
                            {"Don't show", "Always show", "Hold shift"},
                            s.emotesTooltipPreview,
                            [](int index) { return index; },
                            [](auto args) { return args.index; }, false);

    layout.addCheckbox(
        "Only search for emote autocompletion at the start of emote names",
        s.prefixOnlyEmoteCompletion);

    layout.addCheckbox("Show twitch whispers inline", s.inlineWhispers);
    layout.addCheckbox("Highlight received inline whispers",
                       s.highlightInlineWhispers);
    layout.addCheckbox("Load message history on connect",
                       s.loadTwitchMessageHistoryOnConnect);

#ifdef Q_OS_WIN
    layout.addTitle("Browser Integration");
    layout.addDescription("The browser extension replaces the default "
                          "Twitch.tv chat with chatterino.");

    layout.addDescription(
        createNamedLink(CHROME_EXTENSION_LINK, "Download for Google Chrome"));
    layout.addDescription(
        createNamedLink(FIREFOX_EXTENSION_LINK, "Download for Firefox"));
#endif
}  // namespace chatterino

void GeneralPage::initExtra()
{
    /// update cache path
    if (this->cachePath)
    {
        getSettings()->cachePath.connect(
            [cachePath = this->cachePath](const auto &, auto) mutable {
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
