#pragma once

#include "util/Expected.hpp"

#include <QDateTime>
#include <QString>

#include <filesystem>
#include <vector>

class QJsonValue;

namespace chatterino {
class Paths;
}  // namespace chatterino

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
    qint64 fileSize;
    BackupState state;
};

std::vector<BackupFile> findBackupsFor(const QString &directory,
                                       const QString &filename);

struct FileData {
    /// "settings.json", "window-layout.json"
    QString fileName;
    QString directory;
    /// "Settings", "Window layout" etc.
    QString fileKind;
    /// "This file stores..."
    QString fileDescription;
};

void loadSettingFileWithBackups(const FileData &fileData,
                                const std::function<ExpectedStr<void>()> &load);

}  // namespace chatterino::backup

Q_DECLARE_METATYPE(chatterino::backup::BackupFile);
