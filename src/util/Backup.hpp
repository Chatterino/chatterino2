#pragma once

#include "util/Expected.hpp"

#include <QDateTime>
#include <QString>

#include <filesystem>
#include <vector>

class QJsonValue;

namespace chatterino::backup {

bool anyBackupsOf(const QString &directory, const QString &filename);

enum class BackupState : uint8_t {
    /// The backup contains valid JSON
    Ok,
    /// The backup could not be read (e.g. invalid file permissions)
    UnableToRead,
    /// The backup contains invalid JSON
    BadContents,
};

struct BackupFile {
    std::filesystem::path path;
    std::filesystem::path dstPath;
    QDateTime lastModified;
    BackupState state;
};

std::vector<BackupFile> findBackupsFor(const QString &directory,
                                       const QString &filename);

struct RestoreAction {
    enum class Command : uint8_t {
        Copy,
        Move,
    };

    Command command;
    std::filesystem::path src;
    std::filesystem::path dst;

    std::error_code applyOnce() const;
    ExpectedStr<void> applyWithBackoff() const;

    QString toHtml() const;

    QJsonValue toJson() const;
    static std::optional<RestoreAction> fromJson(const QJsonValue &val);
};

QString encodeRestoreActions(const std::vector<RestoreAction> &actions);

void tryApplyActions(const QString &encoded);

}  // namespace chatterino::backup

Q_DECLARE_METATYPE(chatterino::backup::BackupFile);
