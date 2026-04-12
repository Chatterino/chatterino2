// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#define CHATTERINO_REDIRECT_LOG_TO_FILE_ENVVAR "CHATTERINO_REDIRECT_LOG_TO_FILE"

#include <QFile>
#include <QString>

#include <memory>
#include <mutex>

namespace chatterino {

class LoggerToFile
{
public:
    ~LoggerToFile();

    static void enable(const QString &filePath);
    void log(QtMsgType type, const QMessageLogContext &context,
             const QString &msg);

private:
    LoggerToFile(const QString &filePath);

    QFile logFile_;
    QtMessageHandler originalHandler_;
};

}  // namespace chatterino
