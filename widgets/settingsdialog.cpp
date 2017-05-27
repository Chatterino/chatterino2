#include "widgets/settingsdialog.h"
#include "accountmanager.h"
#include "twitch/twitchaccount.h"
#include "widgets/settingsdialogtab.h"
#include "windowmanager.h"

#include <QComboBox>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPalette>
#include <QResource>

namespace chatterino {
namespace widgets {

SettingsDialog::SettingsDialog()
    : _snapshot(SettingsManager::getInstance().createSnapshot())
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    setPalette(palette);

    _pageStack.setObjectName("pages");

    setLayout(&_vbox);

    _vbox.addLayout(&_hbox);

    _vbox.addWidget(&_buttonBox);

    auto tabWidget = new QWidget();
    tabWidget->setObjectName("tabWidget");

    tabWidget->setLayout(&_tabs);
    tabWidget->setFixedWidth(200);

    _hbox.addWidget(tabWidget);
    _hbox.addLayout(&_pageStack);

    _buttonBox.addButton(&_okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    _buttonBox.addButton(&_cancelButton, QDialogButtonBox::ButtonRole::RejectRole);

    QObject::connect(&_okButton, &QPushButton::clicked, this, &SettingsDialog::okButtonClicked);
    QObject::connect(&_cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::cancelButtonClicked);

    _okButton.setText("OK");
    _cancelButton.setText("Cancel");

    resize(600, 500);

    addTabs();
}

void SettingsDialog::addTabs()
{
    SettingsManager &settings = SettingsManager::getInstance();

    QVBoxLayout *vbox;

    // Accounts
    vbox = new QVBoxLayout();

    {
        // add remove buttons
        auto buttonBox = new QDialogButtonBox(this);

        auto addButton = new QPushButton("add", this);
        auto removeButton = new QPushButton("remove", this);

        buttonBox->addButton(addButton, QDialogButtonBox::YesRole);
        buttonBox->addButton(removeButton, QDialogButtonBox::NoRole);

        vbox->addWidget(buttonBox);

        // listview
        auto listWidget = new QListWidget(this);

        listWidget->addItem("xD");
        listWidget->addItem("vi von");
        listWidget->addItem("monkaS");

        for (auto &user : AccountManager::getInstance().getTwitchUsers()) {
            listWidget->addItem(user.getUserName());
        }

        vbox->addWidget(listWidget);
    }

    //    vbox->addStretch(1);
    addTab(vbox, "Accounts", ":/images/Message_16xLG.png");

    // Appearance
    vbox = new QVBoxLayout();

    {
        auto group = new QGroupBox("Application");

        auto form = new QFormLayout();
        auto combo = new QComboBox();
        auto slider = new QSlider(Qt::Horizontal);
        auto font = new QPushButton("select");
        auto compactTabs = createCheckbox("Hide tab X", settings.hideTabX);
        auto hidePreferencesButton = createCheckbox("Hide preferences button (ctrl+p to show)",
                                                    settings.hidePreferencesButton);
        auto hideUserButton = createCheckbox("Hide user button", settings.hideUserButton);

        form->addRow("Theme:", combo);
        form->addRow("Theme color:", slider);
        form->addRow("Font:", font);
        form->addRow("Tabbar:", compactTabs);
        form->addRow("", hidePreferencesButton);
        form->addRow("", hideUserButton);

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
                         [&settings](const QString &value) { settings.theme.set(value); });

        // theme hue
        slider->setMinimum(0);
        slider->setMaximum(1000);

        float hue = settings.themeHue.get();

        slider->setValue(std::min(std::max(hue, (float)0.0), (float)1.0) * 1000);

        QObject::connect(slider, &QSlider::valueChanged, this, [&settings](int value) {
            settings.themeHue.set(value / 1000.0);
            WindowManager::getInstance().updateAll();
        });

        group->setLayout(form);

        vbox->addWidget(group);
    }

    {
        auto group = new QGroupBox("Messages");

        auto v = new QVBoxLayout();
        v->addWidget(createCheckbox("Show timestamp", settings.showTimestamps));
        v->addWidget(createCheckbox("Show seconds in timestamp", settings.showTimestampSeconds));
        v->addWidget(createCheckbox("Allow sending duplicate messages (add a space at the end)",
                                    settings.allowDouplicateMessages));
        v->addWidget(createCheckbox("Seperate messages", settings.seperateMessages));
        v->addWidget(createCheckbox("Show message length", settings.showMessageLength));

        group->setLayout(v);

        vbox->addWidget(group);
    }

    vbox->addStretch(1);

    addTab(vbox, "Appearance", ":/images/AppearanceEditorPart_16x.png");

    // Behaviour
    vbox = new QVBoxLayout();

    {
        auto form = new QFormLayout();

        form->addRow("Window:", createCheckbox("Window always on top", settings.windowTopMost));
        form->addRow("Messages:", createCheckbox("Mention users with a @ (except in commands)",
                                                 settings.mentionUsersWithAt));
        form->addRow("", createCheckbox("Hide input box if empty", settings.hideEmptyInput));
        form->addRow("", createCheckbox("Show last read message indicator",
                                        settings.showLastMessageIndicator));

        //        auto v = new QVBoxLayout();
        //        v->addWidget(new QLabel("Mouse scroll speed"));

        auto scroll = new QSlider(Qt::Horizontal);
        form->addRow("Mouse scroll speed:", scroll);

        //        v->addWidget(scroll);
        //        v->addStretch(1);
        //        vbox->addLayout(v);
        vbox->addLayout(form);
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

    vbox->addWidget(createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));
    vbox->addWidget(createCheckbox("Enable BetterTTV Emotes", settings.enableBttvEmotes));
    vbox->addWidget(createCheckbox("Enable FrankerFaceZ Emotes", settings.enableFfzEmotes));
    vbox->addWidget(createCheckbox("Enable Gif Emotes", settings.enableGifs));
    vbox->addWidget(createCheckbox("Enable Emojis", settings.enableEmojis));

    vbox->addWidget(createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));

    vbox->addStretch(1);
    addTab(vbox, "Emotes", ":/images/Emoji_Color_1F60A_19.png");

    // Ignored Users
    vbox = new QVBoxLayout();
    vbox->addStretch(1);
    addTab(vbox, "Ignored Users", ":/images/StatusAnnotations_Blocked_16xLG_color.png");

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
    _tabs.addStretch(1);
}

void SettingsDialog::addTab(QLayout *layout, QString title, QString imageRes)
{
    auto widget = new QWidget();

    widget->setLayout(layout);

    auto tab = new SettingsDialogTab(this, title, imageRes);

    tab->setWidget(widget);

    _tabs.addWidget(tab, 0, Qt::AlignTop);

    _pageStack.addWidget(widget);

    if (_tabs.count() == 1) {
        select(tab);
    }
}

void SettingsDialog::select(SettingsDialogTab *tab)
{
    _pageStack.setCurrentWidget(tab->getWidget());

    if (_selectedTab != NULL) {
        _selectedTab->setSelected(false);
        _selectedTab->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #555; color: #FFF");
    _selectedTab = tab;
}

void SettingsDialog::showDialog()
{
    static SettingsDialog *instance = new SettingsDialog();

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

/// Widget creation helpers
QCheckBox *SettingsDialog::createCheckbox(const QString &title, Setting<bool> &setting)
{
    auto checkbox = new QCheckBox(title);

    // Set checkbox initial state
    checkbox->setChecked(setting.get());

    QObject::connect(checkbox, &QCheckBox::toggled, this,
                     [&setting](bool state) { setting.set(state); });

    return checkbox;
}

void SettingsDialog::okButtonClicked()
{
    this->close();
}

void SettingsDialog::cancelButtonClicked()
{
    _snapshot.apply();

    this->close();
}

}  // namespace widgets
}  // namespace chatterino
