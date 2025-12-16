#include "util/Backup.hpp"

#include "common/QLogging.hpp"
#include "util/Expected.hpp"
#include "util/FilesystemHelpers.hpp"
#include "widgets/dialogs/RestoreBackupsDialog.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringBuilder>

#include <algorithm>

namespace {

QRegularExpression regexForFile(const QString &file)
{
    return QRegularExpression(QStringView(u"^%1\\.(?:restore-)?bkp-\\d+$")
                                  .arg(QRegularExpression::escape(file)));
}

}  // namespace

namespace chatterino::backup {

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
    for (const auto &entry : fileDir.entryInfoList(QDir::Files, QDir::Time))
    {
        if (!regex.match(entry.fileName()).hasMatch())
        {
            continue;
        }

        BackupState state = BackupState::UnableToRead;
        QFile file(entry.absoluteFilePath());
        if (file.open(QFile::ReadOnly))
        {
            auto ba = file.readAll();
            QJsonParseError err;
            QJsonDocument::fromJson(ba, &err);
            if (err.error == QJsonParseError::NoError)
            {
                state = BackupState::Ok;
            }
            else
            {
                state = BackupState::BadContents;
            }
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

void loadSettingFileWithBackups(const FileData &fileData,
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
