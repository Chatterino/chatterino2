// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/FileLogger.hpp"

#include "common/Env.hpp"

#include <QFileInfo>

#include <cassert>
#include <iostream>

namespace {

void logToFile(QtMsgType type, const QMessageLogContext &context,
               const QString &msg)
{
    chatterino::FileLogger::instance().log(type, context, msg);
}

}  // namespace

namespace chatterino {

FileLogger *FileLogger::INSTANCE = nullptr;

FileLogger::FileLogger()
{
    assert(FileLogger::INSTANCE == nullptr);

    FileLogger::INSTANCE = this;

    const auto &env = Env::get();
    if (!env.logToFile.isEmpty())
    {
        auto result = this->enable(env.logToFile);
        if (!result.has_value())
        {
            auto error = result.error();
            QString errorMessage = QString("Unable to open log file %1. Error "
                                           "reported by the system was: %2")
                                       .arg(error.absFilePath, error.errorDesc);

            std::cerr << errorMessage.constData() << '\n';
        }
    }
}

FileLogger::~FileLogger()
{
    this->disable();
}

void FileLogger::disable()
{
    std::scoped_lock lk(this->logLock);

    if (this->logFile != nullptr)
    {
        qInstallMessageHandler(this->originalHandler);
        this->logFile = nullptr;
    }
}

Expected<void, FileLogger::Error> FileLogger::enable(const QString &filePath)
{
    std::scoped_lock lk(this->logLock);

    QFileInfo finfo(filePath);
    QString absFilePath = finfo.absoluteFilePath();

    if (this->logFile != nullptr)
    {
        QFileInfo finfoCurrent(*this->logFile);

        if (finfoCurrent.absoluteFilePath() == absFilePath)
        {
            // Don't do anything if the current and new target log file are the same
            return {};
        }
    }

    auto f = std::make_unique<QFile>(absFilePath);
    bool success = f->open(QIODevice::WriteOnly);
    if (!success)
    {
        Error error{};
        error.absFilePath = absFilePath;
        error.errorDesc = f->errorString();

        // Abort if we cannot open the log file for writing
        return makeUnexpected(std::move(error));
    }

    if (this->logFile == nullptr)
    {
        this->originalHandler = qInstallMessageHandler(logToFile);
    }

    this->logFile = std::move(f);

    return {};
}

FileLogger &FileLogger::instance()
{
    assert(INSTANCE != nullptr &&
           "Attempted to get instance of FileLogger prior to initializing it");

    return *INSTANCE;
}

void FileLogger::log(QtMsgType type, const QMessageLogContext &context,
                     const QString &msg)
{
    assert(this->logFile != nullptr);

    std::scoped_lock lk(this->logLock);

    auto formatted = qFormatLogMessage(type, context, msg);

    this->logFile->write((formatted + "\n").toUtf8());
    if (this->originalHandler)
    {
        this->originalHandler(type, context, msg);
    }

    this->logFile->flush();
}

}  // namespace chatterino
