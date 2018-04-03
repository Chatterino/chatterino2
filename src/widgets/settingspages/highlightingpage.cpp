#include "highlightingpage.hpp"

#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>

#include "debug/log.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"

#define ENABLE_HIGHLIGHTS "Enable Highlighting"
#define HIGHLIGHT_MSG "Highlight messages containing your name"
#define PLAY_SOUND "Play sound when your name is mentioned"
#define FLASH_TASKBAR "Flash taskbar when your name is mentioned"
#define ALWAYS_PLAY "Always play highlight sound (Even if Chatterino is focused)"

namespace chatterino {
namespace widgets {
namespace settingspages {

HighlightingPage::HighlightingPage()
    : SettingsPage("Highlights", ":/images/notifications.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<HighlightingPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // GENERAL
        layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS, settings.enableHighlights));
        layout.append(this->createCheckBox(HIGHLIGHT_MSG, settings.enableHighlightsSelf));
        layout.append(this->createCheckBox(PLAY_SOUND, settings.enableHighlightSound));
        layout.append(this->createCheckBox(FLASH_TASKBAR, settings.enableHighlightTaskbar));

        auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
        {
            customSound.append(this->createCheckBox("Custom sound", settings.customHighlightSound));
            auto selectFile = customSound.emplace<QPushButton>("Select custom sound file");
            QObject::connect(selectFile.getElement(), &QPushButton::clicked, this,
                             [&settings, this] {
                                 auto fileName = QFileDialog::getOpenFileName(
                                     this, tr("Open Sound"), "", tr("Audio Files (*.mp3 *.wav)"));
                                 settings.pathHighlightSound = fileName;
                             });
        }

        layout.append(createCheckBox(ALWAYS_PLAY, settings.highlightAlwaysPlaySound));

        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Highlights");
            {
                highlights.emplace<QListWidget>().assign(&this->highlightList);

                auto buttons = highlights.emplace<QHBoxLayout>();

                buttons.emplace<QPushButton>("Add").assign(&this->highlightAdd);
                buttons.emplace<QPushButton>("Edit").assign(&this->highlightEdit);
                buttons.emplace<QPushButton>("Remove").assign(&this->highlightRemove);

                this->addHighlightTabSignals();
            }
            // DISABLED USERS
            auto disabledUsers = tabs.appendTab(new QVBoxLayout, "Disabled Users");
            {
                auto text = disabledUsers.emplace<QTextEdit>().getElement();

                QObject::connect(text, &QTextEdit::textChanged, this,
                                 [this] { this->disabledUsersChangedTimer.start(200); });

                QObject::connect(
                    &this->disabledUsersChangedTimer, &QTimer::timeout, this, [text, &settings]() {
                        QStringList list = text->toPlainText().split("\n", QString::SkipEmptyParts);
                        list.removeDuplicates();
                        settings.highlightUserBlacklist = list.join("\n") + "\n";
                    });

                settings.highlightUserBlacklist.connect([=](const QString &str, auto) {
                    text->setPlainText(str);  //
                });
            }
        }
    }

    // ---- misc
    this->disabledUsersChangedTimer.setSingleShot(true);
}

//
// DISCLAIMER:
//
// If you are trying to learn from reading the chatterino code please ignore this segment.
//
void HighlightingPage::addHighlightTabSignals()
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();

    auto addBtn = this->highlightAdd;
    auto editBtn = this->highlightEdit;
    auto delBtn = this->highlightRemove;
    auto highlights = this->highlightList;

    // Open "Add new highlight" dialog
    QObject::connect(addBtn, &QPushButton::clicked, this, [highlights, this, &settings] {
        auto show = new QWidget();
        auto box = new QBoxLayout(QBoxLayout::TopToBottom);

        auto edit = new QLineEdit();
        auto add = new QPushButton("Add");

        auto sound = new QCheckBox("Play sound");
        auto task = new QCheckBox("Flash taskbar");

        // Save highlight
        QObject::connect(add, &QPushButton::clicked, this, [=, &settings] {
            if (edit->text().length()) {
                QString highlightKey = edit->text();
                highlights->addItem(highlightKey);

                auto properties = settings.highlightProperties.getValue();

                messages::HighlightPhrase newHighlightProperty;
                newHighlightProperty.key = highlightKey;
                newHighlightProperty.sound = sound->isChecked();
                newHighlightProperty.alert = task->isChecked();

                properties.push_back(newHighlightProperty);

                settings.highlightProperties = properties;

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

    // Open "Edit selected highlight" dialog
    QObject::connect(editBtn, &QPushButton::clicked, this, [highlights, this, &settings] {
        if (highlights->selectedItems().isEmpty()) {
            // No item selected
            return;
        }

        QListWidgetItem *selectedHighlight = highlights->selectedItems().first();
        QString highlightKey = selectedHighlight->text();
        auto properties = settings.highlightProperties.getValue();
        auto highlightIt = std::find_if(properties.begin(), properties.end(),
                                        [highlightKey](const auto &highlight) {
                                            return highlight.key == highlightKey;  //
                                        });

        if (highlightIt == properties.end()) {
            debug::Log("Unable to find highlight key {} in highlight properties. "
                       "This is weird",
                       highlightKey);
            return;
        }

        messages::HighlightPhrase &selectedSetting = *highlightIt;
        auto show = new QWidget();
        auto box = new QBoxLayout(QBoxLayout::TopToBottom);

        auto edit = new QLineEdit(highlightKey);
        auto apply = new QPushButton("Apply");

        auto sound = new QCheckBox("Play sound");
        sound->setChecked(selectedSetting.sound);
        auto task = new QCheckBox("Flash taskbar");
        task->setChecked(selectedSetting.alert);

        // Apply edited changes
        QObject::connect(apply, &QPushButton::clicked, this, [=, &settings] {
            QString newHighlightKey = edit->text();

            if (newHighlightKey.length() == 0) {
                return;
            }

            auto properties = settings.highlightProperties.getValue();
            auto highlightIt =
                std::find_if(properties.begin(), properties.end(), [=](const auto &highlight) {
                    return highlight.key == highlightKey;  //
                });

            if (highlightIt == properties.end()) {
                debug::Log("Unable to find highlight key {} in highlight properties. "
                           "This is weird",
                           highlightKey);
                return;
            }
            auto &highlightProperty = *highlightIt;
            highlightProperty.key = newHighlightKey;
            highlightProperty.sound = sound->isCheckable();
            highlightProperty.alert = task->isCheckable();

            settings.highlightProperties = properties;

            selectedHighlight->setText(newHighlightKey);
            selectedHighlight->setText(newHighlightKey);

            show->close();
        });

        box->addWidget(edit);
        box->addWidget(apply);
        box->addWidget(sound);
        box->addWidget(task);
        show->setLayout(box);
        show->show();
    });

    // Delete selected highlight
    QObject::connect(delBtn, &QPushButton::clicked, this, [highlights, &settings] {
        if (highlights->selectedItems().isEmpty()) {
            // No highlight selected
            return;
        }

        QListWidgetItem *selectedHighlight = highlights->selectedItems().first();
        QString highlightKey = selectedHighlight->text();

        auto properties = settings.highlightProperties.getValue();
        properties.erase(std::remove_if(properties.begin(), properties.end(),
                                        [highlightKey](const auto &highlight) {
                                            return highlight.key == highlightKey;  //
                                        }),
                         properties.end());

        settings.highlightProperties = properties;

        delete selectedHighlight;
    });
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
