#include "widgets/settingsdialog.hpp"
#include "const.hpp"
#include "debug/log.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/windowmanager.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "twitch/twitchuser.hpp"
#include "widgets/helper/settingsdialogtab.hpp"
#include "widgets/logindialog.hpp"

#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPalette>
#include <QResource>
#include <QTextEdit>

namespace chatterino {
namespace widgets {

SettingsDialog *SettingsDialog::handle = nullptr;

SettingsDialog::SettingsDialog()
    : BaseWidget()
    , usernameDisplayMode(
          "/appearance/messages/usernameDisplayMode",
          twitch::TwitchMessageBuilder::UsernameDisplayMode::UsernameAndLocalizedName)
{
    this->initAsWindow();

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    this->setPalette(palette);

    this->ui.pageStack.setObjectName("pages");

    this->setLayout(&this->ui.vbox);

    this->ui.vbox.addLayout(&this->ui.hbox);

    this->ui.vbox.addWidget(&this->ui.buttonBox);

    auto tabWidget = &ui.tabWidget;
    tabWidget->setObjectName("tabWidget");

    tabWidget->setLayout(&this->ui.tabs);

    this->ui.hbox.addWidget(tabWidget);
    this->ui.hbox.addLayout(&this->ui.pageStack);

    this->ui.buttonBox.addButton(&this->ui.okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    this->ui.buttonBox.addButton(&this->ui.cancelButton, QDialogButtonBox::ButtonRole::RejectRole);

    QObject::connect(&this->ui.okButton, &QPushButton::clicked, this,
                     &SettingsDialog::okButtonClicked);
    QObject::connect(&this->ui.cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::cancelButtonClicked);

    this->ui.okButton.setText("OK");
    this->ui.cancelButton.setText("Cancel");

    this->resize(600, 500);

    this->addTabs();

    this->dpiMultiplierChanged(this->getDpiMultiplier(), this->getDpiMultiplier());
}

SettingsDialog *SettingsDialog::getHandle()
{
    return SettingsDialog::handle;
}

void SettingsDialog::addTabs()
{
    this->addTab(this->createAccountsTab(), "Accounts", ":/images/Message_16xLG.png");

    this->addTab(this->createAppearanceTab(), "Appearance",
                 ":/images/AppearanceEditorPart_16x.png");

    this->addTab(this->createBehaviourTab(), "Behaviour", ":/images/AppearanceEditorPart_16x.png");

    //    this->addTab(this->createCommandsTab(), "Commands",
    //    ":/images/CustomActionEditor_16x.png");

    this->addTab(this->createEmotesTab(), "Emotes", ":/images/Emoji_Color_1F60A_19.png");

    //    this->addTab(this->createIgnoredUsersTab(), "Ignored Users",
    //                 ":/images/StatusAnnotations_Blocked_16xLG_color.png");

    //    this->addTab(this->createIgnoredMessagesTab(), "Ignored Messages",
    //    ":/images/Filter_16x.png");

    //    this->addTab(this->createLinksTab(), "Links", ":/images/VSO_Link_blue_16x.png");

    //    this->addTab(this->createLogsTab(), "Logs", ":/images/VSO_Link_blue_16x.png");

    this->addTab(this->createHighlightingTab(), "Highlighting", ":/images/format_Bold_16xLG.png");

    //    this->addTab(this->createWhispersTab(), "Whispers", ":/images/Message_16xLG.png");

    // Add stretch
    this->ui.tabs.addStretch(1);
}

QVBoxLayout *SettingsDialog::createAccountsTab()
{
    auto layout = new QVBoxLayout();
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();

    // add remove buttons
    auto buttonBox = new QDialogButtonBox(this);

    auto addButton = new QPushButton("Add", this);
    addButton->setToolTip("Log in with a new account");

    auto removeButton = new QPushButton("Remove", this);
    removeButton->setToolTip("Remove selected account");

    connect(addButton, &QPushButton::clicked, []() {
        static auto loginWidget = new LoginWidget();
        loginWidget->show();
    });

    buttonBox->addButton(addButton, QDialogButtonBox::YesRole);
    buttonBox->addButton(removeButton, QDialogButtonBox::NoRole);

    layout->addWidget(buttonBox);

    this->ui.accountSwitchWidget = new AccountSwitchWidget(this);

    connect(removeButton, &QPushButton::clicked, [this]() {
        auto selectedUser = this->ui.accountSwitchWidget->currentItem()->text();
        if (selectedUser == ANONYMOUS_USERNAME_LABEL) {
            // Do nothing
            return;
        }

        singletons::AccountManager::getInstance().Twitch.removeUser(selectedUser);
    });

    layout->addWidget(this->ui.accountSwitchWidget);

    return layout;
}

QVBoxLayout *SettingsDialog::createAppearanceTab()
{
    auto &settings = singletons::SettingManager::getInstance();
    auto layout = this->createTabLayout();

    {
        auto group = new QGroupBox("Application");

        auto form = new QFormLayout();
        auto combo = new QComboBox();

        auto fontLayout = new QHBoxLayout();
        auto fontFamilyLabel = new QLabel("font family, size");
        auto fontButton = new QPushButton("Select");

        fontLayout->addWidget(fontButton);
        fontLayout->addWidget(fontFamilyLabel);

        {
            auto &fontManager = singletons::FontManager::getInstance();

            auto UpdateFontFamilyLabel = [fontFamilyLabel, &fontManager](auto) {
                fontFamilyLabel->setText(
                    QString::fromStdString(fontManager.currentFontFamily.getValue()) + ", " +
                    QString::number(fontManager.currentFontSize) + "pt");
            };

            fontManager.currentFontFamily.connectSimple(UpdateFontFamilyLabel,
                                                        this->managedConnections);
            fontManager.currentFontSize.connectSimple(UpdateFontFamilyLabel,
                                                      this->managedConnections);
        }

        fontButton->connect(fontButton, &QPushButton::clicked, []() {
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

        auto compactTabs = createCheckbox("Hide tab X", settings.hideTabX);
        auto hidePreferencesButton = createCheckbox("Hide preferences button (ctrl+p to show)",
                                                    settings.hidePreferencesButton);
        auto hideUserButton = createCheckbox("Hide user button", settings.hideUserButton);

        form->addRow("Theme:", combo);

        {
            auto hbox = new QHBoxLayout();

            auto slider = new QSlider(Qt::Horizontal);
            // Theme hue
            slider->setMinimum(0);
            slider->setMaximum(1000);

            pajlada::Settings::Setting<double> themeHue("/appearance/theme/hue");

            slider->setValue(std::min(std::max(themeHue.getValue(), 0.0), 1.0) * 1000);

            hbox->addWidget(slider);

            auto button = new QPushButton();
            button->setFlat(true);

            hbox->addWidget(button);

            form->addRow("Theme color:", hbox);

            QObject::connect(slider, &QSlider::valueChanged, this, [button](int value) mutable {
                double newValue = value / 1000.0;
                pajlada::Settings::Setting<double> themeHue("/appearance/theme/hue");

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
        }

        form->addRow("Font:", fontLayout);
        form->addRow("Tab bar:", compactTabs);
        form->addRow("", hidePreferencesButton);
        form->addRow("", hideUserButton);

        {
            // Theme name
            combo->addItems({
                "White",  //
                "Light",  //
                "Dark",   //
                "Black",  //
            });
            // combo->addItem("White");
            // combo->addItem("Light");
            // combo->addItem("Dark");
            // combo->addItem("Black");

            QString currentComboText = QString::fromStdString(
                pajlada::Settings::Setting<std::string>::get("/appearance/theme/name"));

            combo->setCurrentText(currentComboText);

            QObject::connect(combo, &QComboBox::currentTextChanged, this, [](const QString &value) {
                // dirty hack
                singletons::EmoteManager::getInstance().incGeneration();
                pajlada::Settings::Setting<std::string>::set("/appearance/theme/name",
                                                             value.toStdString());
            });
        }

        auto enableSmoothScrolling =
            createCheckbox("Enable smooth scrolling", settings.enableSmoothScrolling);
        form->addRow("Scrolling:", enableSmoothScrolling);

        group->setLayout(form);

        layout->addWidget(group);
    }

    {
        auto group = new QGroupBox("Messages");

        auto v = new QVBoxLayout();
        v->addWidget(createCheckbox("Show timestamp", settings.showTimestamps));
        v->addWidget(createCheckbox("Show seconds in timestamp", settings.showTimestampSeconds));
        v->addWidget(createCheckbox("Show badges", settings.showBadges));
        v->addWidget(createCheckbox("Allow sending duplicate messages (add a space at the end)",
                                    settings.allowDuplicateMessages));
        v->addWidget(createCheckbox("Seperate messages", settings.seperateMessages));
        v->addWidget(createCheckbox("Show message length", settings.showMessageLength));
        v->addLayout(this->createCombobox(
            "Username display mode", this->usernameDisplayMode,
            {"Username (Localized name)", "Username", "Localized name"},
            [](const QString &newValue, pajlada::Settings::Setting<int> &setting) {
                if (newValue == "Username (Localized name)") {
                    setting =
                        twitch::TwitchMessageBuilder::UsernameDisplayMode::UsernameAndLocalizedName;
                } else if (newValue == "Username") {
                    setting = twitch::TwitchMessageBuilder::UsernameDisplayMode::Username;
                } else if (newValue == "Localized name") {
                    setting = twitch::TwitchMessageBuilder::UsernameDisplayMode::LocalizedName;
                }
            }));

        group->setLayout(v);

        layout->addWidget(group);
    }
    return layout;
}

QVBoxLayout *SettingsDialog::createBehaviourTab()
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    auto layout = this->createTabLayout();

    auto form = new QFormLayout();

    form->addRow("Window:",
                 createCheckbox("Window always on top (requires restart)", settings.windowTopMost));
    //        form->addRow("Messages:", createCheckbox("Mention users with a @ (except in
    //        commands)",
    //                                                 settings.mentionUsersWithAt));
    form->addRow("Messages:", createCheckbox("Hide input box if empty", settings.hideEmptyInput));
    form->addRow(
        "", createCheckbox("Show last read message indicator", settings.showLastMessageIndicator));

    auto scroll = new QSlider(Qt::Horizontal);
    form->addRow("Mouse scroll speed:", scroll);

    float currentValue = singletons::SettingManager::getInstance().mouseScrollMultiplier;
    int scrollValue = ((currentValue - 0.1f) / 2.f) * 99.f;
    scroll->setValue(scrollValue);

    connect(scroll, &QSlider::valueChanged, [](int newValue) {
        float mul = static_cast<float>(newValue) / 99.f;
        float newScrollValue = (mul * 2.1f) + 0.1f;
        singletons::SettingManager::getInstance().mouseScrollMultiplier = newScrollValue;
    });

    form->addRow("Streamlink path:", createLineEdit(settings.streamlinkPath));
    form->addRow(this->createCombobox(
        "Preferred quality:", settings.preferredQuality,
        {"Choose", "Source", "High", "Medium", "Low", "Audio only"},
        [](const QString &newValue, pajlada::Settings::Setting<std::string> &setting) {
            setting = newValue.toStdString();
        }));

    layout->addLayout(form);

    return layout;
}

QVBoxLayout *SettingsDialog::createCommandsTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

QVBoxLayout *SettingsDialog::createEmotesTab()
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    auto layout = this->createTabLayout();

    layout->addWidget(createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));
    layout->addWidget(createCheckbox("Enable BetterTTV Emotes", settings.enableBttvEmotes));
    layout->addWidget(createCheckbox("Enable FrankerFaceZ Emotes", settings.enableFfzEmotes));
    layout->addWidget(createCheckbox("Enable Gif Emotes", settings.enableGifAnimations));
    layout->addWidget(createCheckbox("Enable Emojis", settings.enableEmojis));

    layout->addWidget(createCheckbox("Enable Twitch Emotes", settings.enableTwitchEmotes));

    return layout;
}

QVBoxLayout *SettingsDialog::createIgnoredUsersTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

QVBoxLayout *SettingsDialog::createIgnoredMessagesTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

QVBoxLayout *SettingsDialog::createLinksTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

QVBoxLayout *SettingsDialog::createLogsTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

QVBoxLayout *SettingsDialog::createHighlightingTab()
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    auto layout = this->createTabLayout();

    auto highlights = new QListWidget();
    auto highlightUserBlacklist = new QTextEdit();
    globalHighlights = highlights;
    QStringList items = settings.highlightProperties.get().keys();
    highlights->addItems(items);
    highlightUserBlacklist->setText(settings.highlightUserBlacklist.getnonConst());
    auto highlightTab = new QTabWidget();
    auto customSound = new QHBoxLayout();
    auto soundForm = new QFormLayout();
    {
        layout->addWidget(createCheckbox("Enable Highlighting", settings.enableHighlights));
        layout->addWidget(createCheckbox("Highlight messages containing your name",
                                         settings.enableHighlightsSelf));
        layout->addWidget(createCheckbox("Play sound when your name is mentioned",
                                         settings.enableHighlightSound));
        layout->addWidget(createCheckbox("Flash taskbar when your name is mentioned",
                                         settings.enableHighlightTaskbar));
        customSound->addWidget(createCheckbox("Custom sound", settings.customHighlightSound));
        auto selectBtn = new QPushButton("Select");
        QObject::connect(selectBtn, &QPushButton::clicked, this, [&settings, this] {
            auto fileName = QFileDialog::getOpenFileName(this, tr("Open Sound"), "",
                                                         tr("Audio Files (*.mp3 *.wav)"));
            settings.pathHighlightSound.set(fileName);
        });
        customSound->addWidget(selectBtn);
    }

    soundForm->addRow(customSound);

    {
        auto hbox = new QHBoxLayout();
        auto addBtn = new QPushButton("Add");
        auto editBtn = new QPushButton("Edit");
        auto delBtn = new QPushButton("Remove");

        QObject::connect(addBtn, &QPushButton::clicked, this, [highlights, this, &settings] {
            auto show = new QWidget();
            auto box = new QBoxLayout(QBoxLayout::TopToBottom);

            auto edit = new QLineEdit();
            auto add = new QPushButton("Add");

            auto sound = new QCheckBox("Play sound");
            auto task = new QCheckBox("Flash taskbar");

            QObject::connect(add, &QPushButton::clicked, this, [=, &settings] {
                if (edit->text().length()) {
                    highlights->addItem(edit->text());
                    settings.highlightProperties.insertMap(edit->text(), sound->isChecked(),
                                                           task->isChecked());
                    show->close();
                }
            });
            box->addWidget(edit);
            box->addWidget(add);
            box->addWidget(sound);
            box->addWidget(task);
            show->setLayout(box);
            show->show();
        });
        QObject::connect(editBtn, &QPushButton::clicked, this, [highlights, this, &settings] {
            if (!highlights->selectedItems().isEmpty()) {
                auto show = new QWidget();
                auto box = new QBoxLayout(QBoxLayout::TopToBottom);

                auto edit = new QLineEdit();
                edit->setText(highlights->selectedItems().first()->text());
                auto add = new QPushButton("Apply");

                auto sound = new QCheckBox("Play sound");
                auto task = new QCheckBox("Flash taskbar");

                QObject::connect(add, &QPushButton::clicked, this, [=, &settings] {
                    if (edit->text().length()) {
                        settings.highlightProperties.getnonConst().remove(
                            highlights->selectedItems().first()->text());
                        delete highlights->selectedItems().first();
                        highlights->addItem(edit->text());
                        settings.highlightProperties.insertMap(edit->text(), sound->isChecked(),
                                                               task->isChecked());
                        show->close();
                    }
                });
                box->addWidget(edit);
                box->addWidget(add);
                box->addWidget(sound);
                sound->setChecked(settings.highlightProperties.get()
                                      .value(highlights->selectedItems().first()->text())
                                      .first);
                box->addWidget(task);
                task->setChecked(settings.highlightProperties.get()
                                     .value(highlights->selectedItems().first()->text())
                                     .second);
                show->setLayout(box);
                show->show();
            }
        });
        QObject::connect(delBtn, &QPushButton::clicked, this, [highlights, &settings] {
            if (!highlights->selectedItems().isEmpty()) {
                settings.highlightProperties.getnonConst().remove(
                    highlights->selectedItems().first()->text());
                delete highlights->selectedItems().first();
            }
        });
        layout->addLayout(soundForm);
        layout->addWidget(
            createCheckbox("Always play highlight sound (Even if Chatterino is focused)",
                           settings.highlightAlwaysPlaySound));
        auto layoutVbox = new QVBoxLayout();
        auto btnHbox = new QHBoxLayout();

        auto highlightWidget = new QWidget();
        auto btnWidget = new QWidget();

        btnHbox->addWidget(addBtn);
        btnHbox->addWidget(editBtn);
        btnHbox->addWidget(delBtn);
        btnWidget->setLayout(btnHbox);

        layoutVbox->addWidget(highlights);
        layoutVbox->addWidget(btnWidget);
        highlightWidget->setLayout(layoutVbox);

        highlightTab->addTab(highlightWidget, "Highlights");
        highlightTab->addTab(highlightUserBlacklist, "Disabled Users");
        layout->addWidget(highlightTab);

        layout->addLayout(hbox);
    }

    QObject::connect(&this->ui.okButton, &QPushButton::clicked, this, [=, &settings]() {
        QStringList list =
            highlightUserBlacklist->toPlainText().split("\n", QString::SkipEmptyParts);
        list.removeDuplicates();
        settings.highlightUserBlacklist.set(list.join("\n") + "\n");
    });

    settings.highlightUserBlacklist.valueChanged.connect(
        [=](const QString &str) { highlightUserBlacklist->setPlainText(str); });

    return layout;
}

QVBoxLayout *SettingsDialog::createWhispersTab()
{
    auto layout = this->createTabLayout();

    return layout;
}

void SettingsDialog::addTab(QBoxLayout *layout, QString title, QString imageRes)
{
    layout->addStretch(1);

    auto widget = new QWidget();

    widget->setLayout(layout);

    auto tab = new SettingsDialogTab(this, title, imageRes);

    tab->setWidget(widget);

    this->ui.tabs.addWidget(tab, 0, Qt::AlignTop);
    tabs.push_back(tab);

    this->ui.pageStack.addWidget(widget);

    if (this->ui.tabs.count() == 1) {
        this->select(tab);
    }
}

void SettingsDialog::select(SettingsDialogTab *tab)
{
    this->ui.pageStack.setCurrentWidget(tab->getWidget());

    if (this->selectedTab != nullptr) {
        this->selectedTab->setSelected(false);
        this->selectedTab->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #555; color: #FFF");
    this->selectedTab = tab;
}

void SettingsDialog::showDialog(PreferredTab preferredTab)
{
    static SettingsDialog *instance = new SettingsDialog();
    instance->refresh();

    switch (preferredTab) {
        case SettingsDialog::PreferredTab::Accounts: {
            instance->select(instance->tabs.at(0));
        } break;
    }

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void SettingsDialog::refresh()
{
    this->ui.accountSwitchWidget->refresh();

    singletons::SettingManager::getInstance().saveSnapshot();
}

void SettingsDialog::dpiMultiplierChanged(float oldDpi, float newDpi)
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    styleSheet.replace("<font-size>", QString::number((int)(14 * newDpi)));
    styleSheet.replace("<checkbox-size>", QString::number((int)(14 * newDpi)));

    for (SettingsDialogTab *tab : this->tabs) {
        tab->setFixedHeight((int)(30 * newDpi));
    }

    this->setStyleSheet(styleSheet);

    this->ui.tabWidget.setFixedWidth((int)(200 * newDpi));
}

void SettingsDialog::setChildrensFont(QLayout *object, QFont &font, int indent)
{
    //    for (QWidget *widget : this->widgets) {
    //        widget->setFont(font);
    //    }
    //    for (int i = 0; i < object->count(); i++) {
    //        if (object->itemAt(i)->layout()) {
    //            setChildrensFont(object->layout()->itemAt(i)->layout(), font, indent + 2);
    //        }

    //        if (object->itemAt(i)->widget()) {
    //            object->itemAt(i)->widget()->setFont(font);

    //            if (object->itemAt(i)->widget()->layout() &&
    //                !object->itemAt(i)->widget()->layout()->isEmpty()) {
    //                setChildrensFont(object->itemAt(i)->widget()->layout(), font, indent + 2);
    //            }
    //        }
    //    }
}

/// Widget creation helpers
QVBoxLayout *SettingsDialog::createTabLayout()
{
    auto layout = new QVBoxLayout();

    return layout;
}

QCheckBox *SettingsDialog::createCheckbox(const QString &title, Setting<bool> &setting)
{
    auto checkbox = new QCheckBox(title);

    // Set checkbox initial state
    checkbox->setChecked(setting.get());

    QObject::connect(checkbox, &QCheckBox::toggled, this, [&setting](bool state) {
        setting.set(state);  //
    });

    return checkbox;
}

QCheckBox *SettingsDialog::createCheckbox(const QString &title,
                                          pajlada::Settings::Setting<bool> &setting)
{
    auto checkbox = new QCheckBox(title);

    // Set checkbox initial state
    setting.connect([checkbox](const bool &value, auto) {
        checkbox->setChecked(value);  //
    });

    QObject::connect(checkbox, &QCheckBox::toggled, this, [&setting](bool state) {
        qDebug() << "update checkbox value";
        setting = state;  //
    });

    return checkbox;
}

QHBoxLayout *SettingsDialog::createCombobox(
    const QString &title, pajlada::Settings::Setting<int> &setting, QStringList items,
    std::function<void(QString, pajlada::Settings::Setting<int> &)> cb)
{
    auto box = new QHBoxLayout();
    auto label = new QLabel(title);
    auto widget = new QComboBox();
    widget->addItems(items);

    QObject::connect(widget, &QComboBox::currentTextChanged, this,
                     [&setting, cb](const QString &newValue) {
                         cb(newValue, setting);  //
                     });

    box->addWidget(label);
    box->addWidget(widget);

    return box;
}

QHBoxLayout *SettingsDialog::createCombobox(
    const QString &title, pajlada::Settings::Setting<std::string> &setting, QStringList items,
    std::function<void(QString, pajlada::Settings::Setting<std::string> &)> cb)
{
    auto box = new QHBoxLayout();
    auto label = new QLabel(title);
    auto widget = new QComboBox();
    widget->addItems(items);
    widget->setCurrentText(QString::fromStdString(setting.getValue()));

    QObject::connect(widget, &QComboBox::currentTextChanged, this,
                     [&setting, cb](const QString &newValue) {
                         cb(newValue, setting);  //
                     });

    box->addWidget(label);
    box->addWidget(widget);

    return box;
}

QLineEdit *SettingsDialog::createLineEdit(pajlada::Settings::Setting<std::string> &setting)
{
    auto widget = new QLineEdit(QString::fromStdString(setting.getValue()));

    QObject::connect(widget, &QLineEdit::textChanged, this,
                     [&setting](const QString &newValue) { setting = newValue.toStdString(); });

    return widget;
}

void SettingsDialog::okButtonClicked()
{
    this->close();
}

void SettingsDialog::cancelButtonClicked()
{
    auto &settings = singletons::SettingManager::getInstance();

    settings.recallSnapshot();

    QStringList list = settings.highlightProperties.get().keys();
    list.removeDuplicates();
    while (globalHighlights->count() > 0) {
        delete globalHighlights->takeItem(0);
    }
    globalHighlights->addItems(list);

    this->close();
}

}  // namespace widgets
}  // namespace chatterino
