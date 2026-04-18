// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/Expected.hpp"

#include <QFile>
#include <QString>

#include <mutex>

namespace chatterino {

constexpr const char *CHATTERINO_REDIRECT_LOG_TO_FILE_ENVVAR =
    "CHATTERINO_REDIRECT_LOG_TO_FILE";

class FileLogger
{
public:
    struct Error {
        QString absFilePath;
        QString errorDesc;
    };

    FileLogger();

    static FileLogger &instance();

    void disable();
    Expected<void, Error> enable(const QString &filePath);
    void log(QtMsgType type, const QMessageLogContext &context,
             const QString &msg);

private:
    static FileLogger *instance_;
    std::mutex logLock_;

    std::unique_ptr<QFile> logFile_;
    QtMessageHandler originalHandler_;
};

}  // namespace chatterino
