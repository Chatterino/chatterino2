#include "ModerationPage.hpp"

#include "Application.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

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

        auto created = logs.emplace<QLabel>();

        app->settings->logPath.connect([app, created](const QString &logPath, auto) mutable {
            if (logPath == "") {
                created->setText("Logs are saved to " +
                                 CreateLink(app->paths->messageLogDirectory, true));
            } else {
                created->setText("Logs are saved to " + CreateLink(logPath, true));
            }
        });

        created->setTextFormat(Qt::RichText);
        created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                         Qt::LinksAccessibleByKeyboard |
                                         Qt::LinksAccessibleByKeyboard);
        created->setOpenExternalLinks(true);
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

    auto modMode = tabs.appendTab(new QVBoxLayout, "Moderation mode");
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

        // auto modButtons =
        //     modMode.emplace<QGroupBox>("Custom moderator buttons").setLayoutType<QVBoxLayout>();
        // {
        //     auto label2 =
        //         modButtons.emplace<QLabel>("One action per line. {user} will be replaced with the
        //         "
        //                                    "username.<br>Example `/timeout {user} 120`<br>");
        //     label2->setWordWrap(true);

        //     auto text = modButtons.emplace<QTextEdit>().getElement();

        //     text->setPlainText(app->moderationActions->items);

        //     QObject::connect(text, &QTextEdit::textChanged, this,
        //                      [this] { this->itemsChangedTimer.start(200); });

        //     QObject::connect(&this->itemsChangedTimer, &QTimer::timeout, this, [text, app]() {
        //         app->windows->moderationActions = text->toPlainText();
        //     });
        // }

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
