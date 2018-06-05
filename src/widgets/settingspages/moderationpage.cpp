#include "moderationpage.hpp"

#include "application.hpp"
#include "singletons/loggingmanager.hpp"
#include "controllers/taggedusers/taggeduserscontroller.hpp"
#include "controllers/taggedusers/taggedusersmodel.hpp"
#include "singletons/pathmanager.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/editablemodelview.hpp"

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
namespace widgets {
namespace settingspages {

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
    util::LayoutCreator<ModerationPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();

    auto logs = tabs.appendTab(new QVBoxLayout, "Logs");
    {
        // Logs (copied from LoggingMananger)

        auto created = logs.emplace<QLabel>();
        if (app->settings->logPath == "") {
            created->setText("Logs are saved to " +
                             CreateLink(QCoreApplication::applicationDirPath(), true));
        } else {
            created->setText("Logs are saved to " + CreateLink(app->settings->logPath, true));
        }
        created->setTextFormat(Qt::RichText);
        created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                         Qt::LinksAccessibleByKeyboard |
                                         Qt::LinksAccessibleByKeyboard);
        created->setOpenExternalLinks(true);
        logs.append(this->createCheckBox("Enable logging", app->settings->enableLogging));

<
        logs->addStretch(1);

        auto selectDir = layout.emplace<QPushButton>("Set custom logpath");

        // Setting custom logpath
        QObject::connect(selectDir.getElement(), &QPushButton::clicked, this, [
            this, created, app, dirMemory = QString{app->settings->logPath}
        ]() mutable {
            auto dirName = QFileDialog::getExistingDirectory(this);
            created->setText("Logs are saved to " + CreateLink(dirName, true));

            if (dirName == "" && dirMemory == "") {
                created->setText(
                    "Logs are saved to " +
                    CreateLink(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                               true));
            } else if (dirName == "") {
                dirName = dirMemory;
                created->setText("Logs are saved to " + CreateLink(dirName, true));
            }

            app->settings->logPath = dirName;
            dirMemory = dirName;
            app->logging->refreshLoggingPath();

        });
        // Reset custom logpath
        auto resetDir = layout.emplace<QPushButton>("Reset logpath");
        QObject::connect(
            resetDir.getElement(), &QPushButton::clicked, this, [this, created, app]() mutable {
                app->settings->logPath = "";
                created->setText(
                    "Logs are saved to " +
                    CreateLink(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                               true));
                app->logging->refreshLoggingPath();
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

        auto modButtons =
            modMode.emplace<QGroupBox>("Custom moderator buttons").setLayoutType<QVBoxLayout>();
        {
            auto label2 =
                modButtons.emplace<QLabel>("One action per line. {user} will be replaced with the "
                                           "username.<br>Example `/timeout {user} 120`<br>");
            label2->setWordWrap(true);

            auto text = modButtons.emplace<QTextEdit>().getElement();

            text->setPlainText(app->settings->moderationActions);

            QObject::connect(text, &QTextEdit::textChanged, this,
                             [this] { this->itemsChangedTimer.start(200); });

            QObject::connect(&this->itemsChangedTimer, &QTimer::timeout, this, [text, app]() {
                app->settings->moderationActions = text->toPlainText();
            });
        }

        /*auto taggedUsers = tabs.appendTab(new QVBoxLayout, "Tagged users");
        {
            helper::EditableModelView *view = *taggedUsers.emplace<helper::EditableModelView>(
                app->taggedUsers->createModel(nullptr));

            view->setTitles({"Name"});
            view->getTableView()->horizontalHeader()->setStretchLastSection(true);

            view->addButtonPressed.connect([] {
                getApp()->taggedUsers->users.appendItem(
                    controllers::taggedusers::TaggedUser(ProviderId::Twitch, "example", "xD"));
            });
        }*/
    }

    // ---- misc
    this->itemsChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
