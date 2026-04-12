#include "util/LoggerToFile.hpp"

#include <QFileInfo>

namespace {

std::unique_ptr<chatterino::LoggerToFile> LOGGER = nullptr;
std::mutex LOGGER_LOCK;

void logToFile(QtMsgType type, const QMessageLogContext &context,
               const QString &msg)
{
    std::lock_guard<std::mutex> lk(LOGGER_LOCK);

    if (LOGGER == nullptr)
    {
        return;
    }

    LOGGER->log(type, context, msg);
}

}  // namespace

namespace chatterino {

LoggerToFile::LoggerToFile(const QString &filePath)
    : logFile_(filePath)
{
    this->logFile_.open(QIODevice::WriteOnly);

    this->originalHandler_ = qInstallMessageHandler(logToFile);
}

LoggerToFile::~LoggerToFile()
{
    if (this->originalHandler_ != nullptr)
    {
        qInstallMessageHandler(this->originalHandler_);
    }
}

void LoggerToFile::log(QtMsgType /*type*/,
                       const QMessageLogContext & /*context*/,
                       const QString &msg)
{
    this->logFile_.write((msg + "\n").toUtf8());
}

bool LoggerToFile::enable(const QString &filePath)
{
    std::lock_guard<std::mutex> lk(LOGGER_LOCK);

    QFileInfo finfoNew(filePath);
    QString absFilePath = finfoNew.absoluteFilePath();

    if (LOGGER != nullptr)
    {
        QFileInfo finfoCurrent(LOGGER->logFile_);
        if (finfoCurrent.absoluteFilePath() == absFilePath)
        {
            return true;
        }
    }

    // Check that we can actually create the log file
    {
        QFile f(absFilePath);
        bool success = f.open(QIODevice::WriteOnly);
        if (!success)
        {
            return false;
        }
    }

    // Make sure that we destroy any previous logger before creating a new one
    LOGGER = nullptr;

    // make_unique requires the c-tor to be public, at least with VC++ 19.50.(lotta numbers)
    auto *ptr = new LoggerToFile(filePath);
    LOGGER = std::unique_ptr<LoggerToFile>(ptr);

    return true;
}

}  // namespace chatterino
