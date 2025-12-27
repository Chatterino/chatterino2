#include "util/Backup.hpp"

#include "common/QLogging.hpp"
#include "util/Expected.hpp"
#include "util/FilesystemHelpers.hpp"
#include "widgets/dialogs/RestoreBackupsDialog.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringBuilder>
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>

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

// NOLINTBEGIN(readability-identifier-naming, readability-convert-member-functions-to-static)
class ByteArrayStreamWrapper
{
public:
    using Ch = char;

    ByteArrayStreamWrapper(QByteArrayView ba)
        : begin(ba.data())
        , cur(this->begin)
        , end(ba.data() + ba.size())
    {
    }

    /// Read the current character from stream without moving the read cursor.
    Ch Peek() const
    {
        if (this->empty())
        {
            return '\0';
        }
        return *this->cur;
    }

    /// Read the current character from stream and moving the read cursor to next character.
    Ch Take()
    {
        if (this->empty())
        {
            return '\0';
        }
        return *this->cur++;
    }

    /// Get the current read cursor.
    /// @return Number of characters read from start.
    size_t Tell() const
    {
        return this->cur - this->begin;
    }

    // only used for writing
    Ch *PutBegin()
    {
        assert(false);
        return nullptr;
    }
    void Put(Ch /*unused*/)
    {
        assert(false);
    }
    void Flush()
    {
        assert(false);
    }
    size_t PutEnd(Ch * /*unused*/)
    {
        assert(false);
        return 0;
    }

private:
    bool empty() const
    {
        return this->cur == this->end;
    }

    const char *const begin;
    const char *cur;
    const char *const end;
};
// NOLINTEND(readability-identifier-naming, readability-convert-member-functions-to-static)

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
    for (const auto &entry : entries)
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
            rapidjson::BaseReaderHandler<> handler;
            rapidjson::Reader reader;
            ByteArrayStreamWrapper stream(ba);
            auto res = reader.Parse(stream, handler);
            if (res.IsError())
            {
                state = BackupState::BadContents;
            }
            else
            {
                state = BackupState::Ok;
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
