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
#include "util/standarditemhelper.hpp"

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
        //        layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS, settings.enableHighlights));

        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Highlights");
            {
                QTableView *view = *highlights.emplace<QTableView>();
                auto *model = new QStandardItemModel(0, 4, view);
                model->setHeaderData(0, Qt::Horizontal, "Pattern");
                model->setHeaderData(1, Qt::Horizontal, "Flash taskbar");
                model->setHeaderData(2, Qt::Horizontal, "Play sound");
                model->setHeaderData(3, Qt::Horizontal, "Regex");
                view->setModel(model);
                view->setSelectionMode(QAbstractItemView::ExtendedSelection);
                view->setSelectionBehavior(QAbstractItemView::SelectRows);
                view->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

                // own name
                auto *yourName = util::stringItem("Your name (automatic)", false, false);
                yourName->setData(QBrush("#666"), Qt::ForegroundRole);
                yourName->setFlags(yourName->flags() | Qt::ItemIsUserCheckable |
                                   Qt::ItemIsUserCheckable);
                yourName->setData(settings.enableHighlightsSelf ? 2 : 0, Qt::CheckStateRole);
                model->appendRow(
                    {yourName,
                     util::boolItem(settings.enableHighlightTaskbar.getValue(), true, false),
                     util::boolItem(settings.enableHighlightSound.getValue(), true, false),
                     util::emptyItem()});

                // highlight phrases
                // fourtf: could crash
                for (const messages::HighlightPhrase &phrase :
                     settings.highlightProperties.getValue()) {
                    model->appendRow({util::stringItem(phrase.key), util::boolItem(phrase.alert),
                                      util::boolItem(phrase.sound), util::boolItem(phrase.regex)});
                }

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->resizeColumnsToContents();
                    view->setColumnWidth(0, 250);
                });

                auto buttons = highlights.emplace<QHBoxLayout>().withoutMargin();

                QObject::connect(model, &QStandardItemModel::dataChanged,
                                 [model](const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                         const QVector<int> &roles) {
                                     std::vector<messages::HighlightPhrase> phrases;
                                     for (int i = 1; i < model->rowCount(); i++) {
                                         phrases.push_back(messages::HighlightPhrase{
                                             model->item(i, 0)->data(Qt::DisplayRole).toString(),
                                             model->item(i, 1)->data(Qt::CheckStateRole).toBool(),
                                             model->item(i, 2)->data(Qt::CheckStateRole).toBool(),
                                             model->item(i, 3)->data(Qt::CheckStateRole).toBool()});
                                     }
                                     auto &settings = singletons::SettingManager::getInstance();
                                     settings.highlightProperties.setValue(phrases);
                                     settings.enableHighlightsSelf.setValue(
                                         model->item(0, 0)->data(Qt::CheckStateRole).toBool());
                                     settings.enableHighlightTaskbar.setValue(
                                         model->item(0, 1)->data(Qt::CheckStateRole).toBool());
                                     settings.enableHighlightSound.setValue(
                                         model->item(0, 2)->data(Qt::CheckStateRole).toBool());
                                 });

                auto add = buttons.emplace<QPushButton>("Add");
                QObject::connect(*add, &QPushButton::clicked, [model, view] {
                    model->appendRow({util::stringItem(""),
                                      util::boolItem(model->item(model->rowCount() - 1, 1)
                                                         ->data(Qt::CheckStateRole)
                                                         .toBool()),
                                      util::boolItem(model->item(model->rowCount() - 1, 2)
                                                         ->data(Qt::CheckStateRole)
                                                         .toBool()),
                                      util::boolItem(false)});
                    view->scrollToBottom();
                });
                auto remove = buttons.emplace<QPushButton>("Remove");
                QObject::connect(*remove, &QPushButton::clicked, [view, model] {
                    std::vector<int> indices;

                    for (const QModelIndex &index : view->selectionModel()->selectedRows(0)) {
                        indices.push_back(index.row());
                    }

                    std::sort(indices.begin(), indices.end());

                    for (int i = indices.size() - 1; i >= 0; i--) {
                        model->removeRow(indices[i]);
                    }
                });
                buttons->addStretch(1);

                view->hideColumn(3);
            }

            // DISABLED USERS
            // auto disabledUsers = tabs.appendTab(new QVBoxLayout, "Disabled Users");
            // {
            //     auto text = disabledUsers.emplace<QTextEdit>().getElement();

            //     QObject::connect(text, &QTextEdit::textChanged, this,
            //                      [this] { this->disabledUsersChangedTimer.start(200); });

            //     QObject::connect(
            //         &this->disabledUsersChangedTimer, &QTimer::timeout, this, [text, &settings]()
            //         {
            //             QStringList list = text->toPlainText().split("\n",
            //             QString::SkipEmptyParts); list.removeDuplicates();
            //             settings.highlightUserBlacklist = list.join("\n") + "\n";
            //         });

            //     settings.highlightUserBlacklist.connect([=](const QString &str, auto) {
            //         text->setPlainText(str);  //
            //     });
            // }
        }

        // MISC
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
    }

    // ---- misc
    this->disabledUsersChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
