#include "HighlightingPage.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "controllers/highlights/UserHighlightModel.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
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
#define ALWAYS_PLAY "Always play highlight sound (Even if Chatterino is focused)"
#define USERNAME_BOLD "Make @username bold"

namespace chatterino {

HighlightingPage::HighlightingPage()
    : SettingsPage("Highlights", ":/images/notifications.svg")
{
    auto app = getApp();
    LayoutCreator<HighlightingPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // GENERAL
        // layout.append(this->createCheckBox(ENABLE_HIGHLIGHTS, app->settings->enableHighlights));

        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            auto highlights = tabs.appendTab(new QVBoxLayout, "Highlights");
            {
                EditableModelView *view =
                    highlights.emplace<EditableModelView>(app->highlights->createModel(nullptr))
                        .getElement();

                view->setTitles({"Pattern", "Flash taskbar", "Play sound", "Regex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->phrases.appendItem(
                        HighlightPhrase{"my phrase", true, false, false});
                });
            }

            auto disabledUsers = tabs.appendTab(new QVBoxLayout, "Disabled Users");
            {
                EditableModelView *view =
                    disabledUsers
                        .emplace<EditableModelView>(app->highlights->createBlacklistModel(nullptr))
                        .getElement();

                view->setTitles({"Pattern", "Regex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->blacklistedUsers.appendItem(
                        HighlightBlacklistUser{"blacklisted user", false});
                });
            }

            auto pingUsers = tabs.appendTab(new QVBoxLayout, "Highlight on message");
            {
                EditableModelView *view =
                    pingUsers.emplace<EditableModelView>(app->highlights->createUserModel(nullptr))
                        .getElement();

                view->setTitles({"Username", "Flash taskbar", "Play sound", "Regex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->highlights->highlightedUsers.appendItem(
                        HighlightPhrase{"highlighted user", true, false, false});
                });
            }
        }

        // MISC
        auto customSound = layout.emplace<QHBoxLayout>().withoutMargin();
        {
            customSound.append(
                this->createCheckBox("Custom sound", app->settings->customHighlightSound));
            auto selectFile = customSound.emplace<QPushButton>("Select custom sound file");
            QObject::connect(selectFile.getElement(), &QPushButton::clicked, this, [this, app] {
                auto fileName = QFileDialog::getOpenFileName(this, tr("Open Sound"), "",
                                                             tr("Audio Files (*.mp3 *.wav)"));
                app->settings->pathHighlightSound = fileName;
            });
        }

        layout.append(createCheckBox(ALWAYS_PLAY, app->settings->highlightAlwaysPlaySound));
        layout.append(createCheckBox(USERNAME_BOLD, app->settings->usernameBold));
    }

    // ---- misc
    this->disabledUsersChangedTimer.setSingleShot(true);
}

}  // namespace chatterino
