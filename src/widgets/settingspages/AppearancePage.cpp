#include "AppearancePage.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"

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

#define LAST_MSG "Mark the last message you read (dotted line)"

// clang-format off
#define TIMESTAMP_FORMATS "hh:mm a", "h:mm a", "hh:mm:ss a", "h:mm:ss a", "HH:mm", "H:mm", "HH:mm:ss", "H:mm:ss"
// clang-format on

namespace chatterino {

AppearancePage::AppearancePage()
    : SettingsPage("Look", ":/images/theme.svg")
{
    auto app = getApp();
    LayoutCreator<AppearancePage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();

    auto application =
        layout.emplace<QGroupBox>("Application").emplace<QVBoxLayout>().withoutMargin();
    {
        auto form = application.emplace<QFormLayout>();

        auto *theme = this->createComboBox({THEME_ITEMS}, app->themes->themeName);
        QObject::connect(theme, &QComboBox::currentTextChanged,
                         [](const QString &) { getApp()->windows->forceLayoutChannelViews(); });

        form->addRow("Theme:", theme);
        // form->addRow("Theme color:", this->createThemeColorChanger());
        form->addRow("UI Scaling:", this->createUiScaleSlider());
        form->addRow("Font:", this->createFontChanger());

        form->addRow("Tabs:", this->createCheckBox(TAB_X, app->settings->showTabCloseButton));
#ifndef USEWINSDK
        form->addRow("", this->createCheckBox(TAB_PREF, app->settings->hidePreferencesButton));
        form->addRow("", this->createCheckBox(TAB_USER, app->settings->hideUserButton));
#endif

        form->addRow("Scrolling:",
                     this->createCheckBox(SCROLL_SMOOTH, app->settings->enableSmoothScrolling));
        form->addRow("", this->createCheckBox(SCROLL_NEWMSG,
                                              app->settings->enableSmoothScrollingNewMessages));
    }

    auto messages = layout.emplace<QGroupBox>("Messages").emplace<QVBoxLayout>();
    {
        messages.append(this->createCheckBox("Show timestamp", app->settings->showTimestamps));
        auto tbox = messages.emplace<QHBoxLayout>().withoutMargin();
        {
            tbox.emplace<QLabel>("timestamp format (a = am/pm):");
            tbox.append(this->createComboBox({TIMESTAMP_FORMATS}, app->settings->timestampFormat));
            tbox->addStretch(1);
        }

        messages.append(this->createCheckBox("Show badges", app->settings->showBadges));

        {
            auto *combo = new QComboBox(this);
            combo->addItems({"Never", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
                             "13", "14", "15"});

            QObject::connect(combo, &QComboBox::currentTextChanged, [](const QString &str) {
                getApp()->settings->collpseMessagesMinLines = str.toInt();
            });

            auto hbox = messages.emplace<QHBoxLayout>().withoutMargin();
            hbox.emplace<QLabel>("Collapse messages longer than");
            hbox.append(combo);
            hbox.emplace<QLabel>("lines");
        }

        messages.append(this->createCheckBox("Separate messages", app->settings->separateMessages));
        messages.append(this->createCheckBox("Alternate message background color",
                                             app->settings->alternateMessageBackground));
        messages.append(this->createCheckBox("Show message length while typing",
                                             app->settings->showMessageLength));

        messages.append(this->createCheckBox(LAST_MSG, app->settings->showLastMessageIndicator));
    }

    auto emotes = layout.emplace<QGroupBox>("Emotes").setLayoutType<QVBoxLayout>();
    {
        /*
        emotes.append(
            this->createCheckBox("Enable Twitch emotes", app->settings->enableTwitchEmotes));
        emotes.append(this->createCheckBox("Enable BetterTTV emotes for Twitch",
                                           app->settings->enableBttvEmotes));
        emotes.append(this->createCheckBox("Enable FrankerFaceZ emotes for Twitch",
                                           app->settings->enableFfzEmotes));
        emotes.append(this->createCheckBox("Enable emojis", app->settings->enableEmojis));
        */
        emotes.append(
            this->createCheckBox("Enable animations", app->settings->enableGifAnimations));

        auto scaleBox = emotes.emplace<QHBoxLayout>();
        {
            scaleBox.emplace<QLabel>("Emote scale:");

            auto emoteScale = scaleBox.emplace<QSlider>(Qt::Horizontal);
            emoteScale->setMinimum(5);
            emoteScale->setMaximum(50);

            auto scaleLabel = scaleBox.emplace<QLabel>("1.0");
            scaleLabel->setFixedWidth(100);
            QObject::connect(emoteScale.getElement(), &QSlider::valueChanged,
                             [scaleLabel](int value) mutable {
                                 float f = (float)value / 10.f;
                                 scaleLabel->setText(QString::number(f));

                                 getApp()->settings->emoteScale.setValue(f);
                             });

            emoteScale->setValue(std::max<int>(
                5, std::min<int>(50, (int)(app->settings->emoteScale.getValue() * 10.f))));

            scaleLabel->setText(QString::number(app->settings->emoteScale.getValue()));
        }

        {
            auto *combo = new QComboBox(this);
            combo->addItems({"EmojiOne 2", "EmojiOne 3", "Twitter", "Facebook", "Apple", "Google",
                             "Messenger"});

            combo->setCurrentText(getApp()->settings->emojiSet);

            QObject::connect(combo, &QComboBox::currentTextChanged, [](const QString &str) {
                getApp()->settings->emojiSet = str;  //
            });

            auto hbox = emotes.emplace<QHBoxLayout>().withoutMargin();
            hbox.emplace<QLabel>("Emoji set");
            hbox.append(combo);
        }
    }

    layout->addStretch(1);
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
    slider->setValue(WindowManager::clampUiScale(getApp()->settings->uiScale.getValue()));

    label->setMinimumWidth(100);

    QObject::connect(slider, &QSlider::valueChanged,
                     [](auto value) { getApp()->settings->uiScale.setValue(value); });

    getApp()->settings->uiScale.connect(
        [label](auto, auto) { label->setText(QString::number(WindowManager::getUiScaleValue())); },
        this->connections_);

    return layout;
}

}  // namespace chatterino
