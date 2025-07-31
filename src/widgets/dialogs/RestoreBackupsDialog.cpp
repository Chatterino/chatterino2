#include "widgets/dialogs/RestoreBackupsDialog.hpp"

#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Backup.hpp"
#include "util/FilesystemHelpers.hpp"
#include "util/Helpers.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStringBuilder>
#include <QVBoxLayout>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

class RestoreSection : public QWidget
{
public:
    struct Args {
        QString name;
        QString description;
        QString fileName;
        const Paths *paths;
    };

    RestoreSection(const Args &args, QWidget *parent = nullptr);

    std::optional<backup::BackupFile> selectedFile() const;

private:
    QComboBox *combo = nullptr;
};

void applyFont(QWidget *widget, int pointSize, QFont::Weight weight)
{
    auto font = widget->font();
    font.setPointSize(pointSize);
    font.setWeight(weight);
    widget->setFont(font);
}

RestoreSection::RestoreSection(const Args &args, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *header = new QLabel(args.name, this);
    applyFont(header, 16, QFont::Bold);
    layout->addWidget(header);

    auto *description = new QLabel(args.description);
    layout->addWidget(description);

    auto availableBackups =
        backup::findBackupsFor(args.paths->settingsDirectory, args.fileName);
    if (availableBackups.empty())
    {
        layout->addWidget(new QLabel("No backups found!"));
    }
    else
    {
        this->combo = new QComboBox;
        this->combo->addItem("None (keep current)");
        auto dtf = QLocale::system().dateTimeFormat(QLocale::ShortFormat);
        for (const auto &backup : availableBackups)
        {
            this->combo->addItem(
                u"%1 (%2, %3)"_s.arg(stdPathToQString(backup.path.filename()),
                                     backup.lastModified.toString(dtf),
                                     formatFileSize(backup.fileSize)),
                QVariant::fromValue(backup));
        }
        auto *lbl = new QLabel("File to restore:");
        lbl->setBuddy(this->combo);

        auto *inner = new QHBoxLayout;
        inner->addWidget(lbl);
        inner->addWidget(this->combo, 1);
        layout->addLayout(inner);
    }
}

std::optional<backup::BackupFile> RestoreSection::selectedFile() const
{
    if (!this->combo)
    {
        return {};
    }

    auto data = this->combo->currentData();
    if (!data.canConvert<backup::BackupFile>())
    {
        return {};
    }
    return data.value<backup::BackupFile>();
}

std::filesystem::path bestBackupFile(const std::filesystem::path &file)
{
    auto base = file;
    base += ".restore-bkp-";
    std::error_code ec;
    for (size_t i = 0; i < 1024; i++)
    {
        auto path = base;
        path += std::to_string(i);
        if (!std::filesystem::exists(path, ec))
        {
            return path;
        }
    }
    // at this point...
    base += "1024";
    return base;
}

void appendRestoreActions(std::vector<backup::RestoreAction> &actions,
                          const std::optional<backup::BackupFile> &file)
{
    if (!file)
    {
        return;
    }

    using Command = backup::RestoreAction::Command;

    if (QFile::exists(file->dstPath))
    {
        actions.emplace_back(backup::RestoreAction{
            .command = Command::Move,
            .src = file->dstPath,
            .dst = bestBackupFile(file->dstPath),
        });
    }

    actions.emplace_back(backup::RestoreAction{
        .command = Command::Copy,
        .src = file->path,
        .dst = file->dstPath,
    });
}

QString formatPlan(const std::vector<backup::RestoreAction> &plan)
{
    QString buf = u"The current plan is:<ol>"_s;
    for (const auto &action : plan)
    {
        buf.append(u"<li>");
        buf.append(action.toHtml());
        buf.append("</li>");
    }
    buf.append("</ol>Restart and apply plan now?");
    return buf;
}

}  // namespace

namespace chatterino {

RestoreBackupsDialog::RestoreBackupsDialog(const Paths *paths, QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("Restore backups");

    auto *layout = new QVBoxLayout(this);

    auto *mainSettings = new RestoreSection({
        .name = u"Main Settings"_s,
        .description =
            u"This file contains most settings like the account or chat font."_s,
        .fileName = "settings.json",
        .paths = paths,
    });
    layout->addWidget(mainSettings);

    auto *windowLayout = new RestoreSection({
        .name = u"Window Layout"_s,
        .description =
            u"This file contains the arrangement of the windows as well as the tabs and splits."_s,
        .fileName = "window-layout.json",
        .paths = paths,
    });
    layout->addWidget(windowLayout);

    auto *commands = new RestoreSection({
        .name = u"Commands"_s,
        .description = u"This file contains all custom commands."_s,
        .fileName = "commands.json",
        .paths = paths,
    });
    layout->addWidget(commands);

    auto *userData = new RestoreSection({
        .name = u"User Data"_s,
        .description =
            u"This file contains custom data for users like notes and colors."_s,
        .fileName = "user-data.json",
        .paths = paths,
    });
    layout->addWidget(userData);

    auto *buttons =
        new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Abort);
    layout->addWidget(buttons);

    QObject::connect(
        buttons, &QDialogButtonBox::clicked,
        [this, buttons, mainSettings, windowLayout, commands,
         userData](auto *btn) {
            if (btn != buttons->button(QDialogButtonBox::Apply))
            {
                return;
            }

            this->accepted();
            std::vector<backup::RestoreAction> actions;
            appendRestoreActions(actions, mainSettings->selectedFile());
            appendRestoreActions(actions, windowLayout->selectedFile());
            appendRestoreActions(actions, commands->selectedFile());
            appendRestoreActions(actions, userData->selectedFile());
            if (actions.empty())
            {
                this->close();
                return;
            }

            auto res = QMessageBox::question(
                this, "Are you sure?", formatPlan(actions),
                QMessageBox::No | QMessageBox::Yes);
            if (res == QMessageBox::Yes)
            {
                if (restartAppDetatched({
                        "--restore-plan",
                        backup::encodeRestoreActions(actions),
                    }))
                {
                    auto *settings = Settings::maybeInstance();
                    if (settings)
                    {
                        settings->disableSave();
                    }
                    auto *app = tryGetApp();
                    if (app)
                    {
                        app->getCommands()->disableSave();
                        app->getUserData()->disableSave();
                        app->getWindows()->disableSave();
                    }

                    if (app)
                    {
                        QApplication::exit();
                    }
                    else
                    {
                        // we're just starting up -> no eventloop yet
                        _Exit(0);
                    }
                }
                else
                {
                    QMessageBox::critical(this, "Chatterino",
                                          "Failed to restart Chatterino.");
                }
            }
        });
    QObject::connect(buttons, &QDialogButtonBox::rejected, [this] {
        this->rejected();
        this->close();
    });
}

}  // namespace chatterino
