// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/Backup.hpp"

#include "common/QLogging.hpp"
#include "util/Expected.hpp"
#include "util/FilesystemHelpers.hpp"
#include "widgets/dialogs/RestoreBackupsDialog.hpp"

#include <pajlada/settings/settingmanager.hpp>
#include <QDir>
#include <QRegularExpression>

#include <algorithm>

namespace {

QRegularExpression regexForFile(const QString &file)
{
    return QRegularExpression(
        QStringView(u"^%1\\.bkp-\\d+$").arg(QRegularExpression::escape(file)));
}

bool anyBackupsOf(const QString &directory, const QString &filename)
{
    QDir fileDir(directory);
    if (!fileDir.exists())
    {
        return false;
    }

    auto regex = regexForFile(filename);
    return std::ranges::any_of(fileDir.entryList(QDir::Files),
                               [&](const auto &entry) {
                                   return regex.match(entry).hasMatch();
                               });
}

}  // namespace

namespace chatterino::backup {

std::vector<BackupFile> findBackupsFor(const QString &directory,
                                       const QString &filename)
{
    QDir fileDir(directory);
    if (!fileDir.exists())
    {
        return {};
    }
    auto dst = qStringToStdPath(fileDir.filePath(filename));

    auto regex = regexForFile(filename);
    std::vector<BackupFile> backups;
    const auto entries = fileDir.entryInfoList(QDir::Files, QDir::Time);
    auto testSM = pajlada::Settings::SettingManager();
    testSM.saveMethod =
        pajlada::Settings::SettingManager::SaveMethod::SaveManually;

    for (const auto &entry : entries)
    {
        if (!regex.match(entry.fileName()).hasMatch())
        {
            continue;
        }

        BackupState state = BackupState::UnableToRead;
        using LoadError = pajlada::Settings::SettingManager::LoadError;

        auto res = testSM.loadFrom(entry.absoluteFilePath().toStdString());
        switch (res)
        {
            case LoadError::NoError:
                state = BackupState::Ok;
                break;

            case LoadError::CannotOpenFile:
            case LoadError::FileHandleError:
            case LoadError::FileReadError:
            case LoadError::FileSeekError:
                state = BackupState::UnableToRead;
                break;

            case LoadError::JSONParseError:
                state = BackupState::BadContents;
                break;

            case LoadError::SavingFromTemporaryFileFailed:
                // should never happen, temporary file loading/saving is not enabled
                assert(false);
                break;
        }

        backups.emplace_back(BackupFile{
            .path = entry.filesystemCanonicalFilePath(),
            .dstPath = dst,
            .lastModified = entry.lastModified(),
            .fileSize = entry.size(),
            .state = state,
        });
    }

    return backups;
}

void loadWithBackups(const FileData &fileData,
                     const std::function<ExpectedStr<void>()> &load)
{
    while (true)
    {
        auto loadResult = load();
        if (loadResult)
        {
            return;
        }
        qCDebug(chatterinoSettings)
            << fileData.fileKind << "failed to load:" << loadResult.error();

        if (!anyBackupsOf(fileData.directory, fileData.fileName))
        {
            qCDebug(chatterinoSettings)
                << "No backups for" << fileData.fileKind;
            return;
        }

        auto *diag = new RestoreBackupsDialog(fileData, loadResult.error());
        auto ret = diag->exec();  // we need to use exec here to block
        if (ret != QDialog::Accepted)
        {
            return;  // rejected -> don't retry
        }

        qCDebug(chatterinoSettings) << "Retrying to load" << fileData.fileKind;
    }
}

}  // namespace chatterino::backup
