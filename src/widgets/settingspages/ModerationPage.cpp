#include "ModerationPage.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationActionModel.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>

namespace chatterino {

inline QString CreateLink(const QString &url, bool file = false)
{
    if (file) {
        return QString("<a href=\"file:///" + url + "\"><span style=\"color: white;\">" + url +
                       "</span></a>");
    }

    return QString("<a href=\"" + url + "\"><span style=\"color: white;\">" + url + "</span></a>");
}

ModerationPage::ModerationPage()
    : SettingsPage("Moderation", "")
{
    auto app = getApp();
    LayoutCreator<ModerationPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();

    auto logs = tabs.appendTab(new QVBoxLayout, "Logs");
    {
        // Logs (copied from LoggingMananger)

        auto logsPathLabel = logs.emplace<QLabel>();

        app->settings->logPath.connect([app, logsPathLabel](const QString &logPath, auto) mutable {

            QString pathOriginal;

            if (logPath == "") {
                pathOriginal = app->paths->messageLogDirectory;
            } else {
                pathOriginal = logPath;
            }

            QString pathShortened;

            if (pathOriginal.size() > 50) {
                pathShortened = pathOriginal;
                pathShortened.resize(50);
                pathShortened += "...";
            } else {
                pathShortened = pathOriginal;
            }

            pathShortened = "Logs saved at <a href=\"file:///" + pathOriginal +
                            "\"><span style=\"color: white;\">" + pathShortened + "</span></a>";

            logsPathLabel->setText(pathShortened);
            logsPathLabel->setToolTip(pathOriginal);
        });

        logsPathLabel->setTextFormat(Qt::RichText);
        logsPathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                               Qt::LinksAccessibleByKeyboard |
                                               Qt::LinksAccessibleByKeyboard);
        logsPathLabel->setOpenExternalLinks(true);
        logs.append(this->createCheckBox("Enable logging", app->settings->enableLogging));

        logs->addStretch(1);
        auto selectDir = logs.emplace<QPushButton>("Set custom logpath");

        // Setting custom logpath
        QObject::connect(selectDir.getElement(), &QPushButton::clicked, this, [this]() {
            auto app = getApp();
            auto dirName = QFileDialog::getExistingDirectory(this);

            app->settings->logPath = dirName;
        });

        // Reset custom logpath
        auto resetDir = logs.emplace<QPushButton>("Reset logpath");
        QObject::connect(resetDir.getElement(), &QPushButton::clicked, this, []() {
            auto app = getApp();
            app->settings->logPath = "";
        });

        // Logs end
    }

    auto modMode = tabs.appendTab(new QVBoxLayout, "Moderation buttons");
    {
        // clang-format off
        auto label = modMode.emplace<QLabel>("Click the moderation mod button (<img width='18' height='18' src=':/images/moderatormode_disabled.png'>) in a channel that you moderate to enable moderator mode.<br>");
        label->setWordWrap(true);
        label->setStyleSheet("color: #bbb");
        // clang-format on

        //        auto form = modMode.emplace<QFormLayout>();
        //        {
        //            form->addRow("Action on timed out messages (unimplemented):",
        //                         this->createComboBox({"Disable", "Hide"},
        //                         app->settings->timeoutAction));
        //        }

        EditableModelView *view =
            modMode.emplace<EditableModelView>(app->moderationActions->createModel(nullptr))
                .getElement();

        view->setTitles({"Actions"});
        view->getTableView()->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        view->getTableView()->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

        view->addButtonPressed.connect([] {
            getApp()->moderationActions->items.appendItem(ModerationAction("/timeout {user} 300"));
        });

        /*auto taggedUsers = tabs.appendTab(new QVBoxLayout, "Tagged users");
        {
            EditableModelView *view = *taggedUsers.emplace<EditableModelView>(
                app->taggedUsers->createModel(nullptr));

            view->setTitles({"Name"});
            view->getTableView()->horizontalHeader()->setStretchLastSection(true);

            view->addButtonPressed.connect([] {
                getApp()->taggedUsers->users.appendItem(
                    TaggedUser(ProviderId::Twitch, "example", "xD"));
            });
        }*/
    }

    // ---- misc
    this->itemsChangedTimer.setSingleShot(true);
}

}  // namespace chatterino
