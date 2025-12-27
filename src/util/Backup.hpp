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
    qint64 fileSize = 0;
    BackupState state = BackupState::Ok;
};

struct FileData {
    /// "settings.json", "window-layout.json"
    QString fileName;
    QString directory;
    /// "Settings", "Window layout" etc.
    QString fileKind;
    /// "This file stores..."
    QString fileDescription;
};

std::vector<BackupFile> findBackupsFor(const QString &directory,
                                       const QString &filename);

void loadWithBackups(const FileData &fileData,
                     const std::function<ExpectedStr<void>()> &load);

}  // namespace chatterino::backup

Q_DECLARE_METATYPE(chatterino::backup::BackupFile);
