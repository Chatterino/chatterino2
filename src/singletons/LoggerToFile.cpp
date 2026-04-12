#include "singletons/LoggerToFile.hpp"

#include <QFileInfo>

#include <cassert>

namespace {

void logToFile(QtMsgType type, const QMessageLogContext &context,
               const QString &msg)
{
    chatterino::LoggerToFile::instance().log(type, context, msg);
}

}  // namespace

namespace chatterino {

LoggerToFile *LoggerToFile::instance_ = nullptr;

LoggerToFile::LoggerToFile()
    : logFile_(nullptr)
    , originalHandler_(nullptr)
{
    this->instance_ = this;
}

void LoggerToFile::disable()
{
    std::scoped_lock lk(this->logLock_);

    if (this->logFile_ != nullptr)
    {
        qInstallMessageHandler(this->originalHandler_);
        this->logFile_ = nullptr;
    }
}

Expected<void, LoggerToFile::Error> LoggerToFile::enable(
    const QString &filePath)
{
    std::scoped_lock lk(this->logLock_);

    QFileInfo finfo(filePath);
    QString absFilePath = finfo.absoluteFilePath();

    if (this->logFile_ != nullptr)
    {
        QFileInfo finfoCurrent(*this->logFile_);

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
        Error error(absFilePath, f->errorString());

        // Abort if we cannot open the log file for writing
        return makeUnexpected(std::move(error));
    }

    if (this->logFile_ == nullptr)
    {
        this->originalHandler_ = qInstallMessageHandler(logToFile);
    }

    this->logFile_ = std::move(f);

    return {};
}

LoggerToFile &LoggerToFile::instance()
{
    assert(
        instance_ != nullptr &&
        "Attempted to get instance of LoggerToFile prior to initializing it");

    return *instance_;
}

void LoggerToFile::log(QtMsgType type, const QMessageLogContext &context,
                       const QString &msg)
{
    std::scoped_lock lk(this->logLock_);

    auto formatted = qFormatLogMessage(type, context, msg);

    this->logFile_->write((formatted + "\n").toUtf8());
    if (this->originalHandler_)
    {
        this->originalHandler_(type, context, msg);
    }

    this->logFile_->flush();
}

}  // namespace chatterino
