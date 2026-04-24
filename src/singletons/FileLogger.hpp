// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/Expected.hpp"

#include <QFile>
#include <QString>
#include <QtLogging>

#include <memory>
#include <mutex>

namespace chatterino {

class FileLogger
{
public:
    struct Error {
        QString absFilePath;
        QString errorDesc;
    };

    FileLogger();
    ~FileLogger();

    static FileLogger &instance();

    void disable();
    Expected<void, Error> enable(const QString &filePath);
    void log(QtMsgType type, const QMessageLogContext &context,
             const QString &msg);

private:
    static FileLogger *INSTANCE;
    std::mutex logLock;

    std::unique_ptr<QFile> logFile;
    QtMessageHandler originalHandler = nullptr;
};

}  // namespace chatterino
