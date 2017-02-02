#include "widgets/settingsdialog.h"
#include "widgets/settingsdialogtab.h"
#include "windows.h"

#include <QComboBox>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPalette>
#include <QResource>

namespace chatterino {
namespace widgets {

SettingsDialog::SettingsDialog()
    : snapshot(Settings::getInstance().createSnapshot())
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    setPalette(palette);

    pageStack.setObjectName("pages");

    setLayout(&vbox);

    vbox.addLayout(&hbox);

    vbox.addWidget(&buttonBox);

    auto tabWidget = new QWidget();
    tabWidget->setObjectName("tabWidget");

    tabWidget->setLayout(&tabs);
    tabWidget->setFixedWidth(200);

    hbox.addWidget(tabWidget);
    hbox.addLayout(&pageStack);

    buttonBox.addButton(&okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    buttonBox.addButton(&cancelButton,
                        QDialogButtonBox::ButtonRole::RejectRole);

    QObject::connect(&okButton, &QPushButton::clicked, this,
                     &SettingsDialog::okButtonClicked);
    QObject::connect(&cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::cancelButtonClicked);

    okButton.setText("OK");
    cancelButton.setText("Cancel");

    resize(600, 500);

    addTabs();
}

void
SettingsDialog::addTabs()
{
    Settings &settings = Settings::getInstance();

    QVBoxLayout *vbox;

    // Appearance
    vbox = new QVBoxLayout();

    {
        auto group = new QGroupBox("Application");

        auto form = new QFormLayout();
        auto combo = new QComboBox();
        auto slider = new QSlider(Qt::Horizontal);
        auto font = new QPushButton("select");

        form->addRow("Theme:", combo);
        form->addRow("Theme color:", slider);
        form->addRow("Font:", font);

        // theme
        combo->addItem("White");
        combo->addItem("Light");
        combo->addItem("Dark");
        combo->addItem("Black");

        QString theme = settings.theme.get();
        theme = theme.toLower();

        if (theme == "light") {
            combo->setCurrentIndex(0);
        } else if (theme == "white") {
            combo->setCurrentIndex(1);
        } else if (theme == "black") {
            combo->setCurrentIndex(3);
        } else {
            combo->setCurrentIndex(2);
        }

        QObject::connect(combo, &QComboBox::currentTextChanged, this,
                         [this, &settings](const QString &value) {
                             settings.theme.set(value);
                         });

        // theme hue
        slider->setMinimum(0);
        slider->setMaximum(1000);

        float hue = settings.themeHue.get();

        slider->setValue(std::min(std::max(hue, (float)0.0), (float)1.0) *
                         1000);

        QObject::connect(slider, &QSlider::valueChanged, this,
                         [this, &settings](int value) {
                             settings.themeHue.set(value / 1000.0);
                             Windows::updateAll();
                         });

        group->setLayout(form);

        vbox->addWidget(group);
    }

    {
        auto group = new QGroupBox("Messages");

        auto v = new QVBoxLayout();
        v->addWidget(createCheckbox("Show timestamp", settings.showTimestamps));
        v->addWidget(createCheckbox("Show seconds in timestamp",
                                    settings.showTimestampSeconds));
        v->addWidget(createCheckbox(
            "Allow sending duplicate messages (add a space at the end)",
            settings.allowDouplicateMessages));
        v->addWidget(
            createCheckbox("Seperate messages", settings.seperateMessages));
        v->addWidget(
            createCheckbox("Show message length", settings.showMessageLength));

        group->setLayout(v);

        vbox->addWidget(group);
    }

    vbox->addStretch(1);

    addTab(vbox, "Appearance", ":/images/AppearanceEditorPart_16x.png");

    // Behaviour
    vbox = new QVBoxLayout();

    vbox->addWidget(
        createCheckbox("Hide input box if empty", settings.hideEmptyInput));
    vbox->addWidget(
        createCheckbox("Mention users with a @ (except in commands)",
                       settings.mentionUsersWithAt));
    vbox->addWidget(
        createCheckbox("Window always on top", settings.windowTopMost));
    vbox->addWidget(createCheckbox("Show last read message indicator",
                                   settings.showLastMessageIndicator));

    {
        auto v = new QVBoxLayout();
        v->addWidget(new QLabel("Mouse scroll speed"));

        auto scroll = new QSlider(Qt::Horizontal);

        v->addWidget(scroll);
        v->addStretch(1);
    }

    vbox->addStretch(1);

    addTab(vbox, "Behaviour", ":/images/AppearanceEditorPart_16x.png");

    // Commands
    vbox = new QVBoxLayout();

    vbox->addWidget(new QLabel());

    vbox->addStretch(1);

    addTab(vbox, "Commands", ":/images/CustomActionEditor_16x.png");

    // Emotes
    vbox = new QVBoxLayout();

    vbox->addWidget(
        createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));
    vbox->addWidget(
        createCheckbox("Enable BetterTTV Emotes", settings.enableBttvEmotes));
    vbox->addWidget(
        createCheckbox("Enable FrankerFaceZ Emotes", settings.enableFfzEmotes));
    vbox->addWidget(createCheckbox("Enable Gif Emotes", settings.enableGifs));
    vbox->addWidget(createCheckbox("Enable Emojis", settings.enableEmojis));

    vbox->addWidget(
        createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));

    vbox->addStretch(1);
    addTab(vbox, "Emotes", ":/images/Emoji_Color_1F60A_19.png");

    // Ignored Users
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Ignored Users",
           ":/images/StatusAnnotations_Blocked_16xLG_color.png");

    // Ignored Messages
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Ignored Messages", ":/images/Filter_16x.png");

    // Links
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Links", ":/images/VSO_Link_blue_16x.png");

    // Logging
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Logs", ":/images/VSO_Link_blue_16x.png");

    // Highlighting
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Highlighting", ":/images/format_Bold_16xLG.png");

    // Whispers
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Whispers", ":/images/Message_16xLG.png");

    // Add stretch
    tabs.addStretch(1);
}

void
SettingsDialog::addTab(QLayout *layout, QString title, QString imageRes)
{
    auto widget = new QWidget();

    widget->setLayout(layout);

    auto tab = new SettingsDialogTab(this, title, imageRes);

    tab->setWidget(widget);

    tabs.addWidget(tab, 0, Qt::AlignTop);

    pageStack.addWidget(widget);

    if (tabs.count() == 1) {
        select(tab);
    }
}

void
SettingsDialog::select(SettingsDialogTab *tab)
{
    pageStack.setCurrentWidget(tab->getWidget());

    if (selectedTab != NULL) {
        selectedTab->setSelected(false);
        selectedTab->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #555; color: #FFF");
    selectedTab = tab;
}

/// Widget creation helpers
QCheckBox *
SettingsDialog::createCheckbox(const QString &title, Setting<bool> &setting)
{
    auto checkbox = new QCheckBox(title);

    // Set checkbox initial state
    checkbox->setChecked(setting.get());

    QObject::connect(checkbox, &QCheckBox::toggled, this,
                     [&setting, this](bool state) { setting.set(state); });

    return checkbox;
}

void
SettingsDialog::okButtonClicked()
{
    this->close();
}

void
SettingsDialog::cancelButtonClicked()
{
    snapshot.apply();

    this->close();
}

}  // namespace widgets
}  // namespace chatterino
