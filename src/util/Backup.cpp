#include "util/Backup.hpp"

#include "util/Expected.hpp"
#include "util/FilesystemHelpers.hpp"
#include "util/QMagicEnum.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringBuilder>

#include <algorithm>
#include <thread>

namespace {

using namespace chatterino;
using namespace chatterino::backup;

QRegularExpression regexForFile(const QString &file)
{
    return QRegularExpression(QStringView(u"^%1\\.(?:restore-)?bkp-\\d+$")
                                  .arg(QRegularExpression::escape(file)));
}

ExpectedStr<void> applyActionsWithError(const QString &encoded)
{
    auto doc = QJsonDocument::fromJson(encoded.toUtf8());
    if (!doc.isArray())
    {
        return makeUnexpected("Not a valid plan - invalid JSON");
    }

    std::vector<RestoreAction> actions;
    for (const auto value : doc.array())
    {
        auto decoded = RestoreAction::fromJson(value);
        if (!decoded)
        {
            return makeUnexpected("Failed to decode item");
        }
        actions.emplace_back(*std::move(decoded));
    }

    for (const auto &action : actions)
    {
        while (true)
        {
            auto result = action.applyWithBackoff();
            if (result)
            {
                break;
            }
            auto userResponse = QMessageBox::warning(
                nullptr, "Chatterino",
                u"Failed to apply a restore action: " % action.toHtml() %
                    u".<br>Error: " % result.error(),
                QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort);
            if (userResponse == QMessageBox::Ignore)
            {
                break;
            }
            if (userResponse != QMessageBox::Retry)
            {
                return makeUnexpected("Cancelled by user.");
            }
        }
    }

    return {};
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
#if QT_CONFIG(cxx17_filesystem)
            .path = entry.filesystemCanonicalFilePath(),
#else
            .path = qStringToStdPath(entry.canonicalFilePath()),
#endif
            .dstPath = dst,
            .lastModified = entry.lastModified(),
            .fileSize = entry.size(),
            .state = state,
        });
    }

    return backups;
}

std::error_code RestoreAction::applyOnce() const
{
    std::error_code ec;
    switch (this->command)
    {
        case Command::Copy: {
            if (!std::filesystem::copy_file(this->src, this->dst, ec) && !ec)
            {
                ec = std::make_error_code(std::errc::io_error);
            }
            break;
        }
        case Command::Move:
            std::filesystem::rename(this->src, this->dst, ec);
            break;
    }
    return ec;
}

ExpectedStr<void> RestoreAction::applyWithBackoff() const
{
    // in total 4.5s at most
    std::error_code lastError;
    for (size_t i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{100} * i);
        lastError = this->applyOnce();
        if (!lastError)
        {
            return {};
        }
    }
    return makeUnexpected(QString::fromStdString(lastError.message()));
}

QString RestoreAction::toHtml() const
{
    auto src = stdPathToQString(this->src).toHtmlEscaped();
    auto dst = stdPathToQString(this->dst).toHtmlEscaped();
    return qmagicenum::enumName(this->command) % u" <a href=\"" % src % u"\">" %
           src % u"</a> to <a href=\"" % dst % u"\">" % dst % u"</a>";
}

QJsonValue RestoreAction::toJson() const
{
    return QJsonArray{
        qmagicenum::enumNameString(this->command),
        stdPathToQString(this->src),
        stdPathToQString(this->dst),
    };
}

std::optional<RestoreAction> RestoreAction::fromJson(const QJsonValue &val)
{
    const auto arr = val.toArray();
    if (arr.size() != 3)
    {
        return {};
    }

    auto cmd = qmagicenum::enumCast<RestoreAction::Command>(arr[0].toString());
    auto src = arr[1].toString();
    auto dst = arr[2].toString();
    if (!cmd || src.isEmpty() || dst.isEmpty())
    {
        return {};
    }

    return RestoreAction{
        .command = *cmd,
        .src = qStringToStdPath(src),
        .dst = qStringToStdPath(dst),
    };
}

QString encodeRestoreActions(const std::vector<RestoreAction> &actions)
{
    QJsonArray arr;
    for (const auto &action : actions)
    {
        arr.append(action.toJson());
    }
    QJsonDocument doc(arr);

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void tryApplyActions(const QString &encoded)
{
    auto result = applyActionsWithError(encoded);
    if (result)
    {
        QMessageBox::information(nullptr, "Chatterino",
                                 "Successfully restored backups.");
        return;
    }
    QMessageBox::warning(nullptr, "Chatterino",
                         u"Failed to restore backups: " % result.error());
}

}  // namespace chatterino::backup
