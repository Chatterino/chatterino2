#include "ModerationPage.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationActionModel.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "util/Helpers.hpp"
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
#include <QtConcurrent/QtConcurrent>

namespace chatterino {

qint64 dirSize(QString dirPath)
{
    qint64 size = 0;
    QDir dir(dirPath);
    // calculate total size of current directories' files
    QDir::Filters fileFilters = QDir::Files | QDir::System | QDir::Hidden;
    for (QString filePath : dir.entryList(fileFilters))
    {
        QFileInfo fi(dir, filePath);
        size += fi.size();
    }
    // add size of child directories recursively
    QDir::Filters dirFilters =
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden;
    for (QString childDirPath : dir.entryList(dirFilters))
        size += dirSize(dirPath + QDir::separator() + childDirPath);
    return size;
}

QString formatSize(qint64 size)
{
    QStringList units = {"Bytes", "KB", "MB", "GB", "TB", "PB"};
    int i;
    double outputSize = size;
    for (i = 0; i < units.size() - 1; i++)
    {
        if (outputSize < 1024)
            break;
        outputSize = outputSize / 1024;
    }
    return QString("%0 %1").arg(outputSize, 0, 'f', 2).arg(units[i]);
}

QString fetchLogDirectorySize()
{
    QString logPathDirectory = getSettings()->logPath.getValue().isEmpty()
                                   ? getPaths()->messageLogDirectory
                                   : getSettings()->logPath;

    qint64 logsSize = dirSize(logPathDirectory);
    QString logsSizeLabel = "Your logs currently take up ";
    logsSizeLabel += formatSize(logsSize);
    logsSizeLabel += " of space";
    return logsSizeLabel;
}

ModerationPage::ModerationPage()
    : SettingsPage("Moderation", ":/settings/moderation.svg")
{
    auto app = getApp();
    LayoutCreator<ModerationPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();
    this->tabWidget_ = tabs.getElement();

    auto logs = tabs.appendTab(new QVBoxLayout, "Logs");
    {
        logs.append(this->createCheckBox("Enable logging",
                                         getSettings()->enableLogging));
        auto logsPathLabel = logs.emplace<QLabel>();

        // Logs (copied from LoggingMananger)
        getSettings()->logPath.connect([logsPathLabel](const QString &logPath,
                                                       auto) mutable {
            QString pathOriginal =
                logPath.isEmpty() ? getPaths()->messageLogDirectory : logPath;

            QString pathShortened =
                "Logs are saved at <a href=\"file:///" + pathOriginal +
                "\"><span style=\"color: white;\">" +
                shortenString(pathOriginal, 50) + "</span></a>";

            logsPathLabel->setText(pathShortened);
            logsPathLabel->setToolTip(pathOriginal);
        });

        logsPathLabel->setTextFormat(Qt::RichText);
        logsPathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                               Qt::LinksAccessibleByKeyboard);
        logsPathLabel->setOpenExternalLinks(true);

        auto buttons = logs.emplace<QHBoxLayout>().withoutMargin();

        // Select and Reset
        auto selectDir = buttons.emplace<QPushButton>("Select log directory ");
        auto resetDir = buttons.emplace<QPushButton>("Reset");

        getSettings()->logPath.connect(
            [element = resetDir.getElement()](const QString &path) {
                element->setEnabled(!path.isEmpty());
            });

        buttons->addStretch();
        logs->addStretch(1);

        // Show how big (size-wise) the logs are
        auto logsPathSizeLabel = logs.emplace<QLabel>();
        logsPathSizeLabel->setText(
            QtConcurrent::run([] { return fetchLogDirectorySize(); }));

        // Select event
        QObject::connect(
            selectDir.getElement(), &QPushButton::clicked, this,
            [this, logsPathSizeLabel]() mutable {
                auto dirName = QFileDialog::getExistingDirectory(this);

                getSettings()->logPath = dirName;

                // Refresh: Show how big (size-wise) the logs are
                logsPathSizeLabel->setText(
                    QtConcurrent::run([] { return fetchLogDirectorySize(); }));
            });

        buttons->addSpacing(16);

        // Reset custom logpath
        QObject::connect(resetDir.getElement(), &QPushButton::clicked, this,
                         [logsPathSizeLabel]() mutable {
                             getSettings()->logPath = "";

                             // Refresh: Show how big (size-wise) the logs are
                             logsPathSizeLabel->setText(QtConcurrent::run(
                                 [] { return fetchLogDirectorySize(); }));
                         });

    }  // logs end

    auto modMode = tabs.appendTab(new QVBoxLayout, "Moderation buttons");
    {
        // clang-format off
        auto label = modMode.emplace<QLabel>(
            "Moderation mode is enabled by clicking <img width='18' height='18' src=':/buttons/modModeDisabled.png'> in a channel that you moderate.<br><br>"
            "Moderation buttons can be bound to chat commands such as \"/ban {user}\", \"/timeout {user} 1000\", \"/w someusername !report {user} was bad in channel {channel}\" or any other custom text commands.<br>"
            "For deleting messages use /delete {msg-id}.");
        label->setWordWrap(true);
        label->setStyleSheet("color: #bbb");
        // clang-format on

        //        auto form = modMode.emplace<QFormLayout>();
        //        {
        //            form->addRow("Action on timed out messages
        //            (unimplemented):",
        //                         this->createComboBox({"Disable", "Hide"},
        //                         getSettings()->timeoutAction));
        //        }

        EditableModelView *view =
            modMode
                .emplace<EditableModelView>(
                    app->moderationActions->createModel(nullptr))
                .getElement();

        view->setTitles({"Actions"});
        view->getTableView()->horizontalHeader()->setSectionResizeMode(
            QHeaderView::Fixed);
        view->getTableView()->horizontalHeader()->setSectionResizeMode(
            0, QHeaderView::Stretch);

        view->addButtonPressed.connect([] {
            getApp()->moderationActions->items.appendItem(
                ModerationAction("/timeout {user} 300"));
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
    this->itemsChangedTimer_.setSingleShot(true);
}

void ModerationPage::selectModerationActions()
{
    this->tabWidget_->setCurrentIndex(1);
}

}  // namespace chatterino
