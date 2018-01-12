#include "appearancepage.hpp"

#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <util/layoutcreator.hpp>

#define THEME_ITEMS "White", "Light", "Dark", "Black"

#define TAB_X "Hide tab x"
#define TAB_PREF "Hide preferences button (ctrl+p to show)"
#define TAB_USER "Hide user button"

#define SCROLL_SMOOTH "Enable smooth scrolling"
#define SCROLL_NEWMSG "Enable smooth scrolling for new messages"

#define TIMESTAMP_FORMATS "hh:mm a", "h:mm a", "HH:mm", "H:mm"

namespace chatterino {
namespace widgets {
namespace settingspages {

AppearancePage::AppearancePage()
    : SettingsPage("Appearance", ":/images/theme.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<AppearancePage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    auto application =
        layout.emplace<QGroupBox>("Application").emplace<QVBoxLayout>().withoutMargin();
    {
        auto form = application.emplace<QFormLayout>();

        // clang-format off
        form->addRow("Theme:",       this->createComboBox({THEME_ITEMS}, singletons::ThemeManager::getInstance().themeName));
        form->addRow("Theme color:", this->createThemeColorChanger());
        form->addRow("Font:",        this->createFontChanger());

        form->addRow("Tab bar:",     this->createCheckBox(TAB_X,         settings.hideTabX));
        form->addRow("",             this->createCheckBox(TAB_PREF,      settings.hidePreferencesButton));
        form->addRow("",             this->createCheckBox(TAB_USER,      settings.hideUserButton));

        form->addRow("Scrolling:",   this->createCheckBox(SCROLL_SMOOTH, settings.enableSmoothScrolling));
        form->addRow("",             this->createCheckBox(SCROLL_NEWMSG, settings.enableSmoothScrollingNewMessages));
        // clang-format on
    }

    auto messages = layout.emplace<QGroupBox>("Messages").emplace<QVBoxLayout>().withoutMargin();
    {
        messages.append(this->createCheckBox("Show timestamp", settings.showTimestamps));
        auto tbox = messages.emplace<QHBoxLayout>();
        {
            tbox.emplace<QLabel>("timestamp format:");
            tbox.append(this->createComboBox({TIMESTAMP_FORMATS}, settings.timestampFormat));
        }
        messages.append(this->createCheckBox("Show badges", settings.showBadges));
        messages.append(this->createCheckBox("Seperate messages", settings.seperateMessages));
        messages.append(this->createCheckBox("Show message length", settings.showMessageLength));
    }

    layout->addStretch(1);
}

QLayout *AppearancePage::createThemeColorChanger()
{
    QHBoxLayout *layout = new QHBoxLayout;

    auto &themeHue = singletons::ThemeManager::getInstance().themeHue;

    // SLIDER
    QSlider *slider = new QSlider(Qt::Horizontal);
    layout->addWidget(slider);
    slider->setValue(std::min(std::max(themeHue.getValue(), 0.0), 1.0) * 1000);

    // BUTTON
    QPushButton *button = new QPushButton;
    layout->addWidget(button);
    button->setFlat(true);
    button->setFixedWidth(64);

    // SIGNALS
    QObject::connect(slider, &QSlider::valueChanged, this, [button, &themeHue](int value) mutable {
        double newValue = value / 1000.0;

        themeHue.setValue(newValue);

        QPalette pal = button->palette();
        QColor color;
        color.setHsvF(newValue, 1.0, 1.0, 1.0);
        pal.setColor(QPalette::Button, color);
        button->setAutoFillBackground(true);
        button->setPalette(pal);
        button->update();

        // TODO(pajlada): re-implement
        // this->windowManager.updateAll();
    });

    return layout;
}

QLayout *AppearancePage::createFontChanger()
{
    QHBoxLayout *layout = new QHBoxLayout;

    auto &fontManager = singletons::FontManager::getInstance();

    // LABEL
    QLabel *label = new QLabel();
    layout->addWidget(label);

    auto updateFontFamilyLabel = [label, &fontManager](auto) {
        label->setText(QString::fromStdString(fontManager.currentFontFamily.getValue()) + ", " +
                       QString::number(fontManager.currentFontSize) + "pt");
    };

    fontManager.currentFontFamily.connectSimple(updateFontFamilyLabel, this->managedConnections);
    fontManager.currentFontSize.connectSimple(updateFontFamilyLabel, this->managedConnections);

    // BUTTON
    QPushButton *button = new QPushButton("Select");
    layout->addWidget(button);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Policy::Fixed);

    QObject::connect(button, &QPushButton::clicked, []() {
        auto &fontManager = singletons::FontManager::getInstance();
        QFontDialog dialog(fontManager.getFont(singletons::FontManager::Medium, 1.));

        dialog.connect(&dialog, &QFontDialog::fontSelected, [](const QFont &font) {
            auto &fontManager = singletons::FontManager::getInstance();
            fontManager.currentFontFamily = font.family().toStdString();
            fontManager.currentFontSize = font.pointSize();
        });

        dialog.show();
        dialog.exec();
    });

    return layout;
}
}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
