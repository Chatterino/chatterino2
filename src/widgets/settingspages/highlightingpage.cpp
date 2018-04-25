#include "highlightingpage.hpp"

#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>

#include "debug/log.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/tupletablemodel.hpp"

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
                QTableView *view = *highlights.emplace<QTableView>();
                auto *model = new util::TupleTableModel<QString, bool, bool, bool>;
                model->setTitles({"Pattern", "Flash taskbar", "Play sound", "Regex"});

                // fourtf: could crash
                for (const messages::HighlightPhrase &phrase :
                     settings.highlightProperties.getValue()) {
                    model->addRow(phrase.key, phrase.alert, phrase.sound, phrase.regex);
                }

                view->setModel(model);
                view->setSelectionMode(QAbstractItemView::SingleSelection);
                view->setSelectionBehavior(QAbstractItemView::SelectRows);
                view->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->resizeColumnsToContents();
                    view->setColumnWidth(0, 250);
                });

                auto buttons = highlights.emplace<QHBoxLayout>();

                model->itemsChanged.connect([model] {
                    std::vector<messages::HighlightPhrase> phrases;
                    for (int i = 0; i < model->getRowCount(); i++) {
                        auto t = model->getRow(i);
                        phrases.push_back(messages::HighlightPhrase{
                            std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t),
                        });
                    }
                    singletons::SettingManager::getInstance().highlightProperties.setValue(phrases);
                });

                auto add = buttons.emplace<QPushButton>("Add");
                QObject::connect(*add, &QPushButton::clicked,
                                 [model] { model->addRow("", true, false, false); });
                auto remove = buttons.emplace<QPushButton>("Remove");
                QObject::connect(*remove, &QPushButton::clicked, [view, model] {
                    if (view->selectionModel()->hasSelection()) {
                        model->removeRow(view->selectionModel()->selectedRows()[0].row());
                    }
                });

                view->hideColumn(3);
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

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
