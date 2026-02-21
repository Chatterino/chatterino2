// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: CC0-1.0

#include "widgets/dialogs/RestoreBackupsDialog.hpp"

#include "common/QLogging.hpp"
#include "util/Backup.hpp"
#include "util/FilesystemHelpers.hpp"
#include "util/FormatTime.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStringBuilder>
#include <QVBoxLayout>

#include <chrono>
#include <filesystem>

using namespace Qt::Literals;

namespace chatterino {

RestoreBackupsDialog::RestoreBackupsDialog(backup::FileData fileData,
                                           const QString &prevError,
                                           QWidget *parent)
    : QDialog(parent, Qt::Dialog)
    , fileData(std::move(fileData))
    , backupCombo(new QComboBox)
    , showButton(u"Show"_s)
    , corruptedBackupsWarning(
          u"Some backups are damaged or otherwise unreadable."_s)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(u"Chatterino - Restore Backup of " %
                         this->fileData.fileKind % '?');

    auto *layout = new QVBoxLayout(this);

    auto *description =
        new QLabel(u"Chatterino " % this->fileData.fileKind.toLower() %
                   u" failed to load: " % prevError % u"<p>" %
                   this->fileData.fileDescription %
                   u"<p>There are backups of this file.<br>Do you want to "
                   u"restore the selected backup?");
    layout->addWidget(description);

    auto *hbox = new QHBoxLayout;
    hbox->addWidget(&this->backupCombo, 1);
    hbox->addWidget(&this->showButton);
    layout->addLayout(hbox);

    this->corruptedBackupsWarning.hide();
    layout->addWidget(&this->corruptedBackupsWarning);

    layout->addSpacing(10);

    auto *buttons = new QDialogButtonBox;
    layout->addWidget(buttons);
    buttons->addButton(QDialogButtonBox::Ok)->setText(u"Restore Backup"_s);
    buttons->addButton(QDialogButtonBox::Cancel)->setText(u"Discard"_s);

    QObject::connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        auto selected = this->backupCombo.currentData();
        auto *data = get_if<backup::BackupFile>(&selected);
        if (!data)
        {
            return;
        }

        bool retry = true;
        while (retry)
        {
            retry = false;
            std::error_code ec;
            qCDebug(chatterinoSettings)
                << "Copying" << stdPathToQString(data->path) << "to"
                << stdPathToQString(data->dstPath);
            if (!std::filesystem::copy_file(
                    data->path, data->dstPath,
                    std::filesystem::copy_options::overwrite_existing, ec))
            {
                retry = QMessageBox::critical(
                            this, "Failed to restore file",
                            u"Failed to copy '%1' to '%2': %3"_s.arg(
                                stdPathToQString(data->path),
                                stdPathToQString(data->dstPath),
                                QString::fromStdString(ec.message())),
                            QMessageBox::Retry | QMessageBox::Ok) ==
                        QMessageBox::Retry;
            }
        }

        this->accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        auto res = QMessageBox::question(
            this, u"Chatterino - Discard Backup?"_s,
            u"Are you sure you want to discard the backup? Doing so will "_s
            "overwrite and discard any previous settings.");
        if (res == QMessageBox::Yes)
        {
            this->reject();
        }
    });

    QObject::connect(&this->showButton, &QPushButton::clicked, this, [this] {
        auto selected = this->backupCombo.currentData();
        auto *data = get_if<backup::BackupFile>(&selected);
        if (!data)
        {
            return;
        }
        auto url = QUrl::fromLocalFile(stdPathToQString(data->path));
        QDesktopServices::openUrl(url);
    });

    this->refreshBackups();

#ifdef Q_OS_LINUX
    // Needed for Sway to make the dialog floating. See
    // https://github.com/swaywm/sway/issues/3095
    this->setFixedSize(this->minimumSize());
#endif
}

void RestoreBackupsDialog::refreshBackups()
{
    this->backupCombo.clear();

    auto availableBackups = backup::findBackupsFor(this->fileData.directory,
                                                   this->fileData.fileName);
    auto dtf = QLocale::system().dateTimeFormat(QLocale::ShortFormat);
    auto now = QDateTime::currentDateTime();

    bool anyCorrupt = false;
    for (const auto &backup : availableBackups)
    {
        if (backup.state != backup::BackupState::Ok)
        {
            anyCorrupt = true;
            continue;
        }

        QString itemStr = stdPathToQString(backup.path.filename());
        itemStr += u" (";
        itemStr += backup.lastModified.toString(dtf);
        auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(
            now - backup.lastModified);
        if (timeDiff.count() > 0)
        {
            itemStr += u" - ";
            itemStr += formatTime(timeDiff);
            itemStr += u" ago";
        }
        itemStr += ')';

        this->backupCombo.addItem(itemStr, QVariant::fromValue(backup));
    }

    this->corruptedBackupsWarning.setVisible(anyCorrupt);

    bool anyBackups = this->backupCombo.count() > 0;
    auto *okButton = this->dialogButtons.button(QDialogButtonBox::Ok);
    if (okButton)
    {
        okButton->setEnabled(anyBackups);
    }
    this->showButton.setEnabled(anyBackups);
}

}  // namespace chatterino
