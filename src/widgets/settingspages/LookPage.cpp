#include "LookPage.hpp"

#include "Application.hpp"
#include "messages/Image.hpp"
#include "messages/MessageBuilder.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/Line.hpp"

#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

#define THEME_ITEMS "White", "Light", "Dark", "Black"

#define TAB_X "Show tab close button"
#define TAB_PREF "Hide preferences button (ctrl+p to show)"
#define TAB_USER "Hide user button"

// clang-format off
#define TIMESTAMP_FORMATS "hh:mm a", "h:mm a", "hh:mm:ss a", "h:mm:ss a", "HH:mm", "H:mm", "HH:mm:ss", "H:mm:ss"
// clang-format on

#ifdef USEWINSDK
#    define WINDOW_TOPMOST "Window always on top"
#else
#    define WINDOW_TOPMOST "Window always on top (requires restart)"
#endif
#define INPUT_EMPTY "Show input box when empty"
#define LAST_MSG "Mark the last message you read"

namespace chatterino {

LookPage::LookPage()
    : SettingsPage("Look", ":/settings/theme.svg")
{
    this->initializeUi();
}

void LookPage::initializeUi()
{
    LayoutCreator<LookPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    // settings
    auto tabs = layout.emplace<QTabWidget>();

    this->addInterfaceTab(tabs.appendTab(new QVBoxLayout, "Interface"));
    this->addMessageTab(tabs.appendTab(new QVBoxLayout, "Messages"));
    this->addEmoteTab(tabs.appendTab(new QVBoxLayout, "Emotes"));
    this->addSplitHeaderTab(tabs.appendTab(new QVBoxLayout, "Split header"));
    this->addBadgesTab(tabs.appendTab(new QVBoxLayout, "Badges"));

    layout->addStretch(1);

    // preview
    layout.emplace<Line>(false);

    auto channelView = layout.emplace<ChannelView>();
    auto channel = this->createPreviewChannel();
    channelView->setChannel(channel);
    channelView->setScaleIndependantHeight(74);
}

void LookPage::addInterfaceTab(LayoutCreator<QVBoxLayout> layout)
{
    // theme
    {
        auto *theme =
            this->createComboBox({THEME_ITEMS}, getApp()->themes->themeName);
        QObject::connect(theme, &QComboBox::currentTextChanged,
                         [](const QString &) {
                             getApp()->windows->forceLayoutChannelViews();
                         });

        auto box = layout.emplace<QHBoxLayout>().withoutMargin();
        box.emplace<QLabel>("Theme: ");
        box.append(theme);
        box->addStretch(1);
    }

    // ui scale
    {
        auto box = layout.emplace<QHBoxLayout>().withoutMargin();
        box.emplace<QLabel>("Window scale: ");
        box.append(this->createUiScaleSlider());
    }

    layout.append(
        this->createCheckBox(WINDOW_TOPMOST, getSettings()->windowTopMost));

    // --
    layout.emplace<Line>(false);

    // tab x
    layout.append(
        this->createCheckBox(TAB_X, getSettings()->showTabCloseButton));

// show buttons
#ifndef USEWINSDK
    layout.append(
        this->createCheckBox(TAB_PREF, getSettings()->hidePreferencesButton));
    layout.append(
        this->createCheckBox(TAB_USER, getSettings()->hideUserButton));
#endif

    // empty input
    layout.append(
        this->createCheckBox(INPUT_EMPTY, getSettings()->showEmptyInput));
    layout.append(this->createCheckBox("Show message length while typing",
                                       getSettings()->showMessageLength));
    layout->addStretch(1);
}

void LookPage::addMessageTab(LayoutCreator<QVBoxLayout> layout)
{
    // font
    layout.append(this->createFontChanger());

    // bold-slider
    {
        auto box = layout.emplace<QHBoxLayout>().withoutMargin();
        box.emplace<QLabel>("Boldness: ");
        box.append(this->createBoldScaleSlider());
    }
    // --
    layout.emplace<Line>(false);

    // timestamps
    {
        auto box = layout.emplace<QHBoxLayout>().withoutMargin();
        box.append(this->createCheckBox("Show timestamps",
                                        getSettings()->showTimestamps));
        box.append(this->createComboBox({TIMESTAMP_FORMATS},
                                        getSettings()->timestampFormat));
        box->addStretch(1);
    }

    // --
    layout.emplace<Line>(false);

    // seperate
    layout.append(this->createCheckBox("Seperate lines",
                                       getSettings()->separateMessages));

    // alternate
    layout.append(this->createCheckBox(
        "Alternate background", getSettings()->alternateMessageBackground));

    // --
    layout.emplace<Line>(false);

    // lowercase links
    layout.append(this->createCheckBox("Lowercase domains",
                                       getSettings()->enableLowercaseLink));
    // bold usernames
    layout.append(this->createCheckBox("Bold @usernames",
                                       getSettings()->enableUsernameBold));

    // collapsing
    {
        auto *combo = new QComboBox(this);
        combo->addItems({"Never", "2", "3", "4", "5", "6", "7", "8", "9", "10",
                         "11", "12", "13", "14", "15"});

        const auto currentIndex = []() -> int {
            auto val = getSettings()->collpseMessagesMinLines.getValue();
            if (val > 0) {
                --val;
            }
            return val;
        }();
        combo->setCurrentIndex(currentIndex);

        QObject::connect(
            combo, &QComboBox::currentTextChanged, [](const QString &str) {
                getSettings()->collpseMessagesMinLines = str.toInt();
            });

        auto hbox = layout.emplace<QHBoxLayout>().withoutMargin();
        hbox.emplace<QLabel>("Collapse messages longer than");
        hbox.append(combo);
        hbox.emplace<QLabel>("lines");
    }

    // last read message
    this->addLastReadMessageIndicatorPatternSelector(layout);

    // --
    layout->addStretch(1);
}

void LookPage::addEmoteTab(LayoutCreator<QVBoxLayout> layout)
{
    /*
    emotes.append(
        this->createCheckBox("Enable Twitch emotes",
    getSettings()->enableTwitchEmotes));
    emotes.append(this->createCheckBox("Enable BetterTTV emotes for Twitch",
                                       getSettings()->enableBttvEmotes));
    emotes.append(this->createCheckBox("Enable FrankerFaceZ emotes for Twitch",
                                       getSettings()->enableFfzEmotes));
    emotes.append(this->createCheckBox("Enable emojis",
    getSettings()->enableEmojis));
    */
    layout.append(
        this->createCheckBox("Animations", getSettings()->enableGifAnimations));
    layout.append(
        this->createCheckBox("Animations only when chatterino has focus",
                             getSettings()->enableAnimationsWhenFocused));

    auto scaleBox = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        scaleBox.emplace<QLabel>("Size:");

        auto emoteScale = scaleBox.emplace<QSlider>(Qt::Horizontal);
        emoteScale->setMinimum(5);
        emoteScale->setMaximum(50);

        auto scaleLabel = scaleBox.emplace<QLabel>("1.0");
        scaleLabel->setFixedWidth(100);
        QObject::connect(emoteScale.getElement(), &QSlider::valueChanged,
                         [scaleLabel](int value) mutable {
                             float f = float(value) / 10.f;
                             scaleLabel->setText(QString::number(f));

                             getSettings()->emoteScale.setValue(f);
                         });

        emoteScale->setValue(std::max<int>(
            5, std::min<int>(
                   50, int(getSettings()->emoteScale.getValue() * 10.f))));

        scaleLabel->setText(
            QString::number(getSettings()->emoteScale.getValue()));
    }

    {
        auto *combo = new QComboBox(this);
        combo->addItems({"EmojiOne 2", "EmojiOne 3", "Twitter", "Facebook",
                         "Apple", "Google", "Messenger"});

        combo->setCurrentText(getSettings()->emojiSet);

        QObject::connect(combo, &QComboBox::currentTextChanged,
                         [](const QString &str) {
                             getSettings()->emojiSet = str;  //
                         });

        auto hbox = layout.emplace<QHBoxLayout>().withoutMargin();
        hbox.emplace<QLabel>("Emoji set:");
        hbox.append(combo);
    }

    layout->addStretch(1);
}

void LookPage::addSplitHeaderTab(LayoutCreator<QVBoxLayout> layout)
{
    layout.append(
        this->createCheckBox("Show uptime", getSettings()->showUptime));
    layout.append(this->createCheckBox("Show viewer count",
                                       getSettings()->showViewerCount));
    layout.append(this->createCheckBox("Show game", getSettings()->showGame));
    layout.append(this->createCheckBox("Show title", getSettings()->showTitle));

    layout->addStretch(1);
}

void LookPage::addBadgesTab(LayoutCreator<QVBoxLayout> layout)
{
    // layout.append(
    //    this->createCheckBox(("Show all badges"), getSettings()->showBadges));
    auto fastSelection = layout.emplace<QHBoxLayout>();
    {
        auto addAll = fastSelection.emplace<QPushButton>("Enable all");
        QObject::connect(addAll.getElement(), &QPushButton::clicked, this, [] {
            getSettings()->showBadgesGlobalAuthority = true;
            getSettings()->showBadgesChannelAuthority = true;
            getSettings()->showBadgesSubscription = true;
            getSettings()->showBadgesVanity = true;
            getSettings()->showBadgesChatterino = true;
        });
        auto removeAll = fastSelection.emplace<QPushButton>("Disable all");
        QObject::connect(removeAll.getElement(), &QPushButton::clicked, this,
                         [] {
                             getSettings()->showBadgesGlobalAuthority = false;
                             getSettings()->showBadgesChannelAuthority = false;
                             getSettings()->showBadgesSubscription = false;
                             getSettings()->showBadgesVanity = false;
                             getSettings()->showBadgesChatterino = false;
                         });
    }
    layout.emplace<Line>(false);
    layout.append(this->createCheckBox(
        ("Show authority badges (staff, admin, turbo, etc)"),
        getSettings()->showBadgesGlobalAuthority));
    layout.append(
        this->createCheckBox(("Show channel badges (broadcaster, moderator)"),
                             getSettings()->showBadgesChannelAuthority));
    layout.append(this->createCheckBox(("Show subscriber badges "),
                                       getSettings()->showBadgesSubscription));
    layout.append(
        this->createCheckBox(("Show vanity badges (prime, bits, subgifter)"),
                             getSettings()->showBadgesVanity));
    layout.append(this->createCheckBox(("Show chatterino badges"),
                                       getSettings()->showBadgesChatterino));
    layout->addStretch(1);
}

void LookPage::addLastReadMessageIndicatorPatternSelector(
    LayoutCreator<QVBoxLayout> layout)
{
    // combo
    auto *combo = new QComboBox(this);
    combo->addItems({"Dotted line", "Solid line"});

    const auto currentIndex = []() -> int {
        switch (getSettings()->lastMessagePattern.getValue()) {
            case Qt::SolidLine:
                return 1;
            case Qt::VerPattern:
            default:
                return 0;
        }
    }();
    combo->setCurrentIndex(currentIndex);

    QObject::connect(
        combo,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [](int index) {
            getSettings()->lastMessagePattern = [&] {
                switch (index) {
                    case 1:
                        return Qt::SolidPattern;
                    case 0:
                    default:
                        return Qt::VerPattern;
                }
            }();
        });

    // layout
    auto hbox = layout.emplace<QHBoxLayout>().withoutMargin();
    hbox.append(this->createCheckBox(LAST_MSG,
                                     getSettings()->showLastMessageIndicator));
    hbox.append(combo);
    hbox->addStretch(1);
}

ChannelPtr LookPage::createPreviewChannel()
{
    auto channel = ChannelPtr(new Channel("preview", Channel::Type::Misc));

    // clang-format off
    {
        MessageBuilder builder;
        builder.emplace<TimestampElement>(QTime(8, 13, 42));
        builder.emplace<ImageElement>(Image::fromPixmap(getApp()->resources->twitch.moderator), MessageElementFlag::BadgeChannelAuthority);
        builder.emplace<ImageElement>(Image::fromPixmap(getApp()->resources->twitch.subscriber, 0.25), MessageElementFlag::BadgeSubscription);
        builder.emplace<TextElement>("username1:", MessageElementFlag::Username, QColor("#0094FF"), FontStyle::ChatMediumBold);
        builder.emplace<TextElement>("This is a preview message", MessageElementFlag::Text);
        builder.emplace<ImageElement>(Image::fromPixmap(getApp()->resources->pajaDank, 0.25), MessageElementFlag::AlwaysShow);
        builder.emplace<TextElement>("@fourtf", MessageElementFlag::BoldUsername, MessageColor::Text, FontStyle::ChatMediumBold);
        builder.emplace<TextElement>("@fourtf", MessageElementFlag::NonBoldUsername);
        channel->addMessage(builder.release());
    }
    {
        MessageBuilder message;
        message.emplace<TimestampElement>(QTime(8, 15, 21));
        message.emplace<ImageElement>(Image::fromPixmap(getApp()->resources->twitch.broadcaster), MessageElementFlag::BadgeChannelAuthority);
        message.emplace<TextElement>("username2:", MessageElementFlag::Username, QColor("#FF6A00"), FontStyle::ChatMediumBold);
        message.emplace<TextElement>("This is another one", MessageElementFlag::Text);
        // message.emplace<ImageElement>(Image::fromNonOwningPixmap(&getApp()->resources->ppHop), MessageElementFlag::BttvEmote);
        message.emplace<TextElement>("www.fourtf.com", MessageElementFlag::LowercaseLink, MessageColor::Link)->setLink(Link(Link::Url, "https://www.fourtf.com"));
        message.emplace<TextElement>("wWw.FoUrTf.CoM", MessageElementFlag::OriginalLink, MessageColor::Link)->setLink(Link(Link::Url, "https://www.fourtf.com"));
        channel->addMessage(message.release());
    }
    // clang-format on

    return channel;
}

QLayout *LookPage::createThemeColorChanger()
{
    auto app = getApp();
    QHBoxLayout *layout = new QHBoxLayout;

    auto &themeHue = app->themes->themeHue;

    // SLIDER
    QSlider *slider = new QSlider(Qt::Horizontal);
    layout->addWidget(slider);
    slider->setValue(
        int(std::min(std::max(themeHue.getValue(), 0.0), 1.0) * 100));

    // BUTTON
    QPushButton *button = new QPushButton;
    layout->addWidget(button);
    button->setFlat(true);
    button->setFixedWidth(64);

    auto setButtonColor = [button, app](int value) mutable {
        double newValue = value / 100.0;
        app->themes->themeHue.setValue(newValue);

        QPalette pal = button->palette();
        QColor color;
        color.setHsvF(newValue, 1.0, 1.0, 1.0);
        pal.setColor(QPalette::Button, color);
        button->setAutoFillBackground(true);
        button->setPalette(pal);
        button->update();
    };

    // SIGNALS
    QObject::connect(slider, &QSlider::valueChanged, this, setButtonColor);

    setButtonColor(themeHue * 100);

    return layout;
}

QLayout *LookPage::createFontChanger()
{
    auto app = getApp();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);

    // LABEL
    QLabel *label = new QLabel();
    layout->addWidget(label);

    auto updateFontFamilyLabel = [=](auto) {
        label->setText(
            "Font (" +
            QString::fromStdString(app->fonts->chatFontFamily.getValue()) +
            ", " + QString::number(app->fonts->chatFontSize) + "pt)");
    };

    app->fonts->chatFontFamily.connectSimple(updateFontFamilyLabel,
                                             this->managedConnections_);
    app->fonts->chatFontSize.connectSimple(updateFontFamilyLabel,
                                           this->managedConnections_);

    // BUTTON
    QPushButton *button = new QPushButton("Select");
    layout->addWidget(button);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Policy::Fixed);

    QObject::connect(button, &QPushButton::clicked, [=]() {
        QFontDialog dialog(app->fonts->getFont(FontStyle::ChatMedium, 1.));

        dialog.setWindowFlag(Qt::WindowStaysOnTopHint);

        dialog.connect(
            &dialog, &QFontDialog::fontSelected, [=](const QFont &font) {
                app->fonts->chatFontFamily = font.family().toStdString();
                app->fonts->chatFontSize = font.pointSize();
            });

        dialog.show();
        dialog.exec();
    });

    layout->addStretch(1);

    return layout;
}

QLayout *LookPage::createUiScaleSlider()
{
    auto layout = new QHBoxLayout();
    auto slider = new QSlider(Qt::Horizontal);
    auto label = new QLabel();
    layout->addWidget(slider);
    layout->addWidget(label);

    slider->setMinimum(WindowManager::uiScaleMin);
    slider->setMaximum(WindowManager::uiScaleMax);
    slider->setValue(
        WindowManager::clampUiScale(getSettings()->uiScale.getValue()));

    label->setMinimumWidth(100);

    QObject::connect(slider, &QSlider::valueChanged, [](auto value) {
        getSettings()->uiScale.setValue(value);
    });

    getSettings()->uiScale.connect(
        [label](auto, auto) {
            label->setText(QString::number(WindowManager::getUiScaleValue()));
        },
        this->connections_);

    return layout;
}

QLayout *LookPage::createBoldScaleSlider()
{
    auto layout = new QHBoxLayout();
    auto slider = new QSlider(Qt::Horizontal);
    auto label = new QLabel();

    layout->addWidget(slider);
    layout->addWidget(label);

    slider->setMinimum(50);
    slider->setMaximum(100);
    slider->setValue(getSettings()->boldScale.getValue());

    label->setMinimumWidth(100);

    QObject::connect(slider, &QSlider::valueChanged, [](auto value) {
        getSettings()->boldScale.setValue(value);
    });
    // show value
    // getSettings()->boldScale.connect(
    //    [label](auto, auto) {
    //        label->setText(QString::number(getSettings()->boldScale.getValue()));
    //    },
    //    this->connections_);

    QPushButton *button = new QPushButton("Reset");
    layout->addWidget(button);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Policy::Fixed);

    QObject::connect(button, &QPushButton::clicked, [=]() {
        getSettings()->boldScale.setValue(57);
        slider->setValue(57);
    });

    return layout;
}

}  // namespace chatterino
