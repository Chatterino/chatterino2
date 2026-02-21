// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: CC0-1.0

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

/// A backup file (e.g. `settings.json.bkp-7`) and its state.
struct BackupFile {
    std::filesystem::path path;
    std::filesystem::path dstPath;
    QDateTime lastModified;
    qint64 fileSize = 0;
    BackupState state = BackupState::Ok;
};

/// Specifies where to load the file from and descriptions about the file and its contents.
struct FileData {
    /// "settings.json", "window-layout.json"
    QString fileName;
    QString directory;
    /// "Settings", "Window layout" etc.
    QString fileKind;
    /// "This file stores..."
    QString fileDescription;
};

/// Find a list of backups for the given `filename` in the given `directory`.
std::vector<BackupFile> findBackupsFor(const QString &directory,
                                       const QString &filename);

/// Attempt to load the file described in `fileData` using the `load` param.
///
/// If the load fails and any backups are available, spawn a restore backups dialog.
void loadWithBackups(const FileData &fileData,
                     const std::function<ExpectedStr<void>()> &load);

}  // namespace chatterino::backup

Q_DECLARE_METATYPE(chatterino::backup::BackupFile);
