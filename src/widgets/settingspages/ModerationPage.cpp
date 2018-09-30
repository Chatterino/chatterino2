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
    for (QString filePath : dir.entryList(fileFilters)) {
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
    for (i = 0; i < units.size() - 1; i++) {
        if (outputSize < 1024) break;
        outputSize = outputSize / 1024;
    }
    return QString("%0 %1").arg(outputSize, 0, 'f', 2).arg(units[i]);
}

QString fetchLogDirectorySize()
{
    QString logPathDirectory;
    if (getSettings()->logPath == "") {
        logPathDirectory = getPaths()->messageLogDirectory;
    } else {
        logPathDirectory = getSettings()->logPath;
    }
    qint64 logsSize = dirSize(logPathDirectory);
    QString logsSizeLabel = "Your logs currently take up ";
    logsSizeLabel += formatSize(logsSize);
    logsSizeLabel += " of space";
    return logsSizeLabel;
}

ModerationPage::ModerationPage()
    : SettingsPage("Moderation", "")
{
    auto app = getApp();
    LayoutCreator<ModerationPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();

    auto logs = tabs.appendTab(new QVBoxLayout, "Logs");
    {
        auto logsPathLabel = logs.emplace<QLabel>();

        // Show how big (size-wise) the logs are
        auto logsPathSizeLabel = logs.emplace<QLabel>();
        logsPathSizeLabel->setText(
            QtConcurrent::run([] { return fetchLogDirectorySize(); }));

        // Logs (copied from LoggingMananger)
        getSettings()->logPath.connect(
            [logsPathLabel](const QString &logPath, auto) mutable {
                QString pathOriginal;

                if (logPath == "") {
                    pathOriginal = getPaths()->messageLogDirectory;
                } else {
                    pathOriginal = logPath;
                }

                QString pathShortened =
                    "Logs saved at <a href=\"file:///" + pathOriginal +
                    "\"><span style=\"color: white;\">" +
                    shortenString(pathOriginal, 50) + "</span></a>";

                logsPathLabel->setText(pathShortened);
                logsPathLabel->setToolTip(pathOriginal);
            });

        logsPathLabel->setTextFormat(Qt::RichText);
        logsPathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                               Qt::LinksAccessibleByKeyboard |
                                               Qt::LinksAccessibleByKeyboard);
        logsPathLabel->setOpenExternalLinks(true);
        logs.append(this->createCheckBox("Enable logging",
                                         getSettings()->enableLogging));

        logs->addStretch(1);
        auto selectDir = logs.emplace<QPushButton>("Set custom logpath");

        // Setting custom logpath
        QObject::connect(
            selectDir.getElement(), &QPushButton::clicked, this,
            [this, logsPathSizeLabel]() mutable {
                auto dirName = QFileDialog::getExistingDirectory(this);

                getSettings()->logPath = dirName;

                // Refresh: Show how big (size-wise) the logs are
                logsPathSizeLabel->setText(
                    QtConcurrent::run([] { return fetchLogDirectorySize(); }));
            });

        // Reset custom logpath
        auto resetDir = logs.emplace<QPushButton>("Reset logpath");
        QObject::connect(resetDir.getElement(), &QPushButton::clicked, this,
                         [logsPathSizeLabel]() mutable {
                             getSettings()->logPath = "";

                             // Refresh: Show how big (size-wise) the logs are
                             logsPathSizeLabel->setText(QtConcurrent::run(
                                 [] { return fetchLogDirectorySize(); }));
                         });

        // Logs end
    }

    auto modMode = tabs.appendTab(new QVBoxLayout, "Moderation buttons");
    {
        // clang-format off
        auto label = modMode.emplace<QLabel>("Click the moderation mod button (<img width='18' height='18' src=':/buttons/modModeDisabled.png'>) in a channel that you moderate to enable moderator mode.<br>");
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

}  // namespace chatterino
