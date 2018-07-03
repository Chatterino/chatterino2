#include "AppearancePage.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
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

#define TAB_X "Show close button"
#define TAB_PREF "Hide preferences button (ctrl+p to show)"
#define TAB_USER "Hide user button"

#define SCROLL_SMOOTH "Enable smooth scrolling"
#define SCROLL_NEWMSG "Enable smooth scrolling for new messages"

// clang-format off
#define TIMESTAMP_FORMATS "hh:mm a", "h:mm a", "hh:mm:ss a", "h:mm:ss a", "HH:mm", "H:mm", "HH:mm:ss", "H:mm:ss"
// clang-format on

namespace chatterino {

AppearancePage::AppearancePage()
    : SettingsPage("Look", ":/images/theme.svg")
{
    LayoutCreator<AppearancePage> layoutCreator(this);

    auto xd = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    // settings
    auto scroll = xd.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto &layout = *widget.setLayoutType<QVBoxLayout>().withoutMargin();

    this->addApplicationGroup(layout);
    this->addMessagesGroup(layout);
    this->addEmotesGroup(layout);

    // preview
    xd.emplace<Line>(false);

    auto channelView = xd.emplace<ChannelView>();
    auto channel = this->createPreviewChannel();
    channelView->setChannel(channel);
    channelView->setScaleIndependantHeight(64);

    layout.addStretch(1);
}

void AppearancePage::addApplicationGroup(QVBoxLayout &layout)
{
    auto box = LayoutCreator<QVBoxLayout>(&layout)
                   .emplace<QGroupBox>("Application")
                   .emplace<QVBoxLayout>()
                   .withoutMargin();

    auto form = box.emplace<QFormLayout>();

    // theme
    auto *theme = this->createComboBox({THEME_ITEMS}, getApp()->themes->themeName);
    QObject::connect(theme, &QComboBox::currentTextChanged,
                     [](const QString &) { getApp()->windows->forceLayoutChannelViews(); });

    form->addRow("Theme:", theme);

    // ui scale
    form->addRow("UI Scaling:", this->createUiScaleSlider());

    // font
    form->addRow("Font:", this->createFontChanger());

    // tab x
    form->addRow("Tabs:", this->createCheckBox(TAB_X, getSettings()->showTabCloseButton));

// show buttons
#ifndef USEWINSDK
    form->addRow("", this->createCheckBox(TAB_PREF, app->settings->hidePreferencesButton));
    form->addRow("", this->createCheckBox(TAB_USER, app->settings->hideUserButton));
#endif

    // scrolling
    form->addRow("Scrolling:",
                 this->createCheckBox(SCROLL_SMOOTH, getSettings()->enableSmoothScrolling));
    form->addRow(
        "", this->createCheckBox(SCROLL_NEWMSG, getSettings()->enableSmoothScrollingNewMessages));
}

void AppearancePage::addMessagesGroup(QVBoxLayout &layout)
{
    auto box =
        LayoutCreator<QVBoxLayout>(&layout).emplace<QGroupBox>("Messages").emplace<QVBoxLayout>();

    // timestamps
    box.append(this->createCheckBox("Show timestamps", getSettings()->showTimestamps));
    auto tbox = box.emplace<QHBoxLayout>().withoutMargin();
    {
        tbox.emplace<QLabel>("Timestamp format (a = am/pm):");
        tbox.append(this->createComboBox({TIMESTAMP_FORMATS}, getSettings()->timestampFormat));
        tbox->addStretch(1);
    }

    // badges
    box.append(this->createCheckBox("Show badges", getSettings()->showBadges));

    // collapsing
    {
        auto *combo = new QComboBox(this);
        combo->addItems(
            {"Never", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"});

        QObject::connect(combo, &QComboBox::currentTextChanged, [](const QString &str) {
            getSettings()->collpseMessagesMinLines = str.toInt();
        });

        auto hbox = box.emplace<QHBoxLayout>().withoutMargin();
        hbox.emplace<QLabel>("Collapse messages longer than");
        hbox.append(combo);
        hbox.emplace<QLabel>("lines");
    }

    // seperate
    box.append(this->createCheckBox("Separation lines", getSettings()->separateMessages));

    // alternate
    box.append(this->createCheckBox("Alternate background colors",
                                    getSettings()->alternateMessageBackground));
}

void AppearancePage::addEmotesGroup(QVBoxLayout &layout)
{
    auto box = LayoutCreator<QVBoxLayout>(&layout)
                   .emplace<QGroupBox>("Emotes")
                   .setLayoutType<QVBoxLayout>();

    /*
    emotes.append(
        this->createCheckBox("Enable Twitch emotes", app->settings->enableTwitchEmotes));
    emotes.append(this->createCheckBox("Enable BetterTTV emotes for Twitch",
                                       app->settings->enableBttvEmotes));
    emotes.append(this->createCheckBox("Enable FrankerFaceZ emotes for Twitch",
                                       app->settings->enableFfzEmotes));
    emotes.append(this->createCheckBox("Enable emojis", app->settings->enableEmojis));
    */
    box.append(this->createCheckBox("Animated emotes", getSettings()->enableGifAnimations));

    auto scaleBox = box.emplace<QHBoxLayout>();
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

        emoteScale->setValue(
            std::max<int>(5, std::min<int>(50, int(getSettings()->emoteScale.getValue() * 10.f))));

        scaleLabel->setText(QString::number(getSettings()->emoteScale.getValue()));
    }

    {
        auto *combo = new QComboBox(this);
        combo->addItems(
            {"EmojiOne 2", "EmojiOne 3", "Twitter", "Facebook", "Apple", "Google", "Messenger"});

        combo->setCurrentText(getSettings()->emojiSet);

        QObject::connect(combo, &QComboBox::currentTextChanged, [](const QString &str) {
            getSettings()->emojiSet = str;  //
        });

        auto hbox = box.emplace<QHBoxLayout>().withoutMargin();
        hbox.emplace<QLabel>("Emoji set:");
        hbox.append(combo);
    }
}

ChannelPtr AppearancePage::createPreviewChannel()
{
    auto channel = ChannelPtr(new Channel("preview", Channel::Misc));

    {
        auto message = MessagePtr(new Message());
        message->addElement(new ImageElement(getApp()->resources->badgeModerator,
                                             MessageElement::BadgeChannelAuthority));
        message->addElement(new ImageElement(getApp()->resources->badgeSubscriber,
                                             MessageElement::BadgeSubscription));
        message->addElement(new TimestampElement());
        message->addElement(new TextElement("username1:", MessageElement::Username,
                                            QColor("#0094FF"), FontStyle::ChatMediumBold));
        message->addElement(new TextElement("This is a preview message :)", MessageElement::Text));
        channel->addMessage(message);
    }
    {
        auto message = MessagePtr(new Message());
        message->addElement(new ImageElement(getApp()->resources->badgePremium,
                                             MessageElement::BadgeChannelAuthority));
        message->addElement(new TimestampElement());
        message->addElement(new TextElement("username2:", MessageElement::Username,
                                            QColor("#FF6A00"), FontStyle::ChatMediumBold));
        message->addElement(new TextElement("This is another one :)", MessageElement::Text));
        channel->addMessage(message);
    }

    return channel;
}

QLayout *AppearancePage::createThemeColorChanger()
{
    auto app = getApp();
    QHBoxLayout *layout = new QHBoxLayout;

    auto &themeHue = app->themes->themeHue;

    // SLIDER
    QSlider *slider = new QSlider(Qt::Horizontal);
    layout->addWidget(slider);
    slider->setValue(int(std::min(std::max(themeHue.getValue(), 0.0), 1.0) * 100));

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

QLayout *AppearancePage::createFontChanger()
{
    auto app = getApp();

    QHBoxLayout *layout = new QHBoxLayout;

    // LABEL
    QLabel *label = new QLabel();
    layout->addWidget(label);

    auto updateFontFamilyLabel = [=](auto) {
        label->setText(QString::fromStdString(app->fonts->chatFontFamily.getValue()) + ", " +
                       QString::number(app->fonts->chatFontSize) + "pt");
    };

    app->fonts->chatFontFamily.connectSimple(updateFontFamilyLabel, this->managedConnections);
    app->fonts->chatFontSize.connectSimple(updateFontFamilyLabel, this->managedConnections);

    // BUTTON
    QPushButton *button = new QPushButton("Select");
    layout->addWidget(button);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Policy::Fixed);

    QObject::connect(button, &QPushButton::clicked, [=]() {
        QFontDialog dialog(app->fonts->getFont(Fonts::ChatMedium, 1.));

        dialog.setWindowFlag(Qt::WindowStaysOnTopHint);

        dialog.connect(&dialog, &QFontDialog::fontSelected, [=](const QFont &font) {
            app->fonts->chatFontFamily = font.family().toStdString();
            app->fonts->chatFontSize = font.pointSize();
        });

        dialog.show();
        dialog.exec();
    });

    return layout;
}

QLayout *AppearancePage::createUiScaleSlider()
{
    auto layout = new QHBoxLayout();
    auto slider = new QSlider(Qt::Horizontal);
    auto label = new QLabel();
    layout->addWidget(slider);
    layout->addWidget(label);

    slider->setMinimum(WindowManager::uiScaleMin);
    slider->setMaximum(WindowManager::uiScaleMax);
    slider->setValue(WindowManager::clampUiScale(getSettings()->uiScale.getValue()));

    label->setMinimumWidth(100);

    QObject::connect(slider, &QSlider::valueChanged,
                     [](auto value) { getSettings()->uiScale.setValue(value); });

    getSettings()->uiScale.connect(
        [label](auto, auto) { label->setText(QString::number(WindowManager::getUiScaleValue())); },
        this->connections_);

    return layout;
}

}  // namespace chatterino
