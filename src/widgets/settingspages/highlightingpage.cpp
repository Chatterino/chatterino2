#include "highlightingpage.hpp"

#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
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
                QTableView *view = *highlights.emplace<QTableView>();
                auto *model = new QStandardItemModel(0, 4, view);
                // model->setTitles({"Pattern", "Flash taskbar", "Play sound", "Regex"});

                // fourtf: could crash
                for (const messages::HighlightPhrase &phrase :
                     settings.highlightProperties.getValue()) {
                    auto *item1 = new QStandardItem(phrase.key);
                    auto *item2 = new QStandardItem();
                    item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item2->setData(phrase.alert, Qt::CheckStateRole);
                    auto *item3 = new QStandardItem();
                    item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item3->setData(phrase.sound, Qt::CheckStateRole);
                    auto *item4 = new QStandardItem();
                    item4->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item4->setData(phrase.regex, Qt::CheckStateRole);
                    model->appendRow({item1, item2, item3, item4});
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

                QObject::connect(
                    model, &QStandardItemModel::dataChanged,
                    [model](const QModelIndex &topLeft, const QModelIndex &bottomRight,
                            const QVector<int> &roles) {
                        std::vector<messages::HighlightPhrase> phrases;
                        for (int i = 0; i < model->rowCount(); i++) {
                            phrases.push_back(messages::HighlightPhrase{
                                model->item(i, 0)->data(Qt::DisplayRole).toString(),
                                model->item(i, 1)->data(Qt::CheckStateRole).toBool(),
                                model->item(i, 2)->data(Qt::CheckStateRole).toBool(),
                                model->item(i, 3)->data(Qt::CheckStateRole).toBool()});
                        }
                        singletons::SettingManager::getInstance().highlightProperties.setValue(
                            phrases);
                    });

                auto add = buttons.emplace<QPushButton>("Add");
                QObject::connect(*add, &QPushButton::clicked, [model] {
                    auto *item1 = new QStandardItem();
                    auto *item2 = new QStandardItem();
                    item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item2->setData(true, Qt::CheckStateRole);
                    auto *item3 = new QStandardItem();
                    item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item3->setData(true, Qt::CheckStateRole);
                    auto *item4 = new QStandardItem();
                    item4->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item4->setData(false, Qt::CheckStateRole);
                    model->appendRow({item1, item2, item3, item4});
                });
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
