#include "HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "util/Log.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFileDialog>
#include <QHeaderView>
#include <QListWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>

#define ENABLE_HIGHLIGHTS "Enable Highlighting"
#define HIGHLIGHT_MSG "Highlight messages containing your name"
#define PLAY_SOUND "Play sound when your name is mentioned"
#define FLASH_TASKBAR "Flash taskbar when your name is mentioned"
#define ALWAYS_PLAY \
    "Always play highlight sound (Even if Chatterino is focused)"

namespace chatterino
{
    HighlightingPage::HighlightingPage()
        : SettingsPage("Highlights", ":/settings/notifications.svg")
    {
        auto app = getApp();
        LayoutCreator<HighlightingPage> layoutCreator(this);

        auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
        {
            // GENERAL
            // layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS,
            // getSettings()->enableHighlights));

            // TABS
            auto tabs = layout.emplace<QTabWidget>();
            {
                // HIGHLIGHTS
                auto highlights = tabs.appendTab(new QVBoxLayout, "Phrases");
                {
                    EditableModelView* view =
                        highlights
                            .emplace<EditableModelView>(
                                app->highlights->createModel(nullptr))
                            .getElement();

                    view->setTitles({"Pattern", "Flash\ntaskbar", "Play\nsound",
                        "Enable\nregex"});
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(QHeaderView::Fixed);
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(0, QHeaderView::Stretch);

                    // fourtf: make class extrend BaseWidget and add this to
                    // dpiChanged
                    QTimer::singleShot(1, [view] {
                        view->getTableView()->resizeColumnsToContents();
                        view->getTableView()->setColumnWidth(0, 200);
                    });

                    view->addButtonPressed.connect([] {
                        getApp()->highlights->phrases.appendItem(
                            HighlightPhrase{"my phrase", true, false, false});
                    });
                }

                auto pingUsers = tabs.appendTab(new QVBoxLayout, "Users");
                {
                    EditableModelView* view =
                        pingUsers
                            .emplace<EditableModelView>(
                                app->highlights->createUserModel(nullptr))
                            .getElement();

                    view->setTitles({"Username", "Flash\ntaskbar",
                        "Play\nsound", "Enable\nregex"});
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(QHeaderView::Fixed);
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(0, QHeaderView::Stretch);

                    // fourtf: make class extrend BaseWidget and add this to
                    // dpiChanged
                    QTimer::singleShot(1, [view] {
                        view->getTableView()->resizeColumnsToContents();
                        view->getTableView()->setColumnWidth(0, 200);
                    });

                    view->addButtonPressed.connect([] {
                        getApp()->highlights->highlightedUsers.appendItem(
                            HighlightPhrase{
                                "highlighted user", true, false, false});
                    });
                }

                auto disabledUsers =
                    tabs.appendTab(new QVBoxLayout, "Excluded Users");
                {
                    EditableModelView* view =
                        disabledUsers
                            .emplace<EditableModelView>(
                                app->highlights->createBlacklistModel(nullptr))
                            .getElement();

                    view->setTitles({"Pattern", "Enable\nregex"});
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(QHeaderView::Fixed);
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(0, QHeaderView::Stretch);

                    // fourtf: make class extrend BaseWidget and add this to
                    // dpiChanged
                    QTimer::singleShot(1, [view] {
                        view->getTableView()->resizeColumnsToContents();
                        view->getTableView()->setColumnWidth(0, 200);
                    });

                    view->addButtonPressed.connect([] {
                        getApp()->highlights->blacklistedUsers.appendItem(
                            HighlightBlacklistUser{"blacklisted user", false});
                    });
                }
            }

            // MISC
            auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
            {
                customSound.append(this->createCheckBox(
                    "Custom sound", getSettings()->customHighlightSound));
                auto selectFile = customSound.emplace<QPushButton>(
                    "Select custom sound file");
                QObject::connect(selectFile.getElement(), &QPushButton::clicked,
                    this, [this] {
                        auto fileName =
                            QFileDialog::getOpenFileName(this, tr("Open Sound"),
                                "", tr("Audio Files (*.mp3 *.wav)"));
                        getSettings()->pathHighlightSound = fileName;
                    });
            }

            layout.append(createCheckBox(
                ALWAYS_PLAY, getSettings()->highlightAlwaysPlaySound));
            layout.append(
                createCheckBox("Flash taskbar only stops highlighting when "
                               "chatterino is focused",
                    getSettings()->longAlerts));
        }

        // ---- misc
        this->disabledUsersChangedTimer_.setSingleShot(true);
    }
}  // namespace chatterino
