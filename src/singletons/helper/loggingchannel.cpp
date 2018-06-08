#include "loggingchannel.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/settingsmanager.hpp"

#include <QDir>

#include <ctime>

namespace chatterino {
namespace singletons {

QByteArray endline("\n");

LoggingChannel::LoggingChannel(const QString &_channelName)
    : channelName(_channelName)
{
    if (this->channelName.startsWith("/whispers")) {
        this->subDirectory = "Whispers";
    } else if (channelName.startsWith("/mentions")) {
        this->subDirectory = "Mentions";
    } else {
        this->subDirectory = QStringLiteral("Channels") + QDir::separator() + channelName;
    }

    auto app = getApp();

    app->settings->logPath.connect([this](const QString &logPath, auto) {
        auto app = getApp();

        if (logPath.isEmpty()) {
            this->baseDirectory = app->paths->logsFolderPath;
        } else {
            this->baseDirectory = logPath;
        }

        this->openLogFile();
    });
}

LoggingChannel::~LoggingChannel()
{
    this->appendLine(this->generateClosingString());
    this->fileHandle.close();
}

void LoggingChannel::openLogFile()
{
    QDateTime now = QDateTime::currentDateTime();
    this->dateString = this->generateDateString(now);

    if (this->fileHandle.isOpen()) {
        this->fileHandle.flush();
        this->fileHandle.close();
    }

    QString baseFileName = this->channelName + "-" + this->dateString + ".log";

    QString directory = this->baseDirectory + QDir::separator() + this->subDirectory;

    if (!QDir().mkpath(directory)) {
        debug::Log("Unable to create logging path");
        return;
    }

    // Open file handle to log file of current date
    QString fileName = directory + QDir::separator() + baseFileName;
    debug::Log("Logging to {}", fileName);
    this->fileHandle.setFileName(fileName);

    this->fileHandle.open(QIODevice::Append);

    this->appendLine(this->generateOpeningString(now));
}

void LoggingChannel::addMessage(std::shared_ptr<messages::Message> message)
{
    QDateTime now = QDateTime::currentDateTime();

    QString messageDateString = this->generateDateString(now);
    if (messageDateString != this->dateString) {
        this->dateString = messageDateString;
        this->openLogFile();
    }

    QString str;
    str.append('[');
    str.append(now.toString("HH:mm:ss"));
    str.append("] ");

    str.append(message->searchText);
    str.append(endline);

    this->appendLine(str);
}

QString LoggingChannel::generateOpeningString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Start logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss "));
    ret.append(now.timeZoneAbbreviation());
    ret.append(endline);

    return ret;
}

QString LoggingChannel::generateClosingString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Stop logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss"));
    ret.append(now.timeZoneAbbreviation());
    ret.append(endline);

    return ret;
}

void LoggingChannel::appendLine(const QString &line)
{
    this->fileHandle.write(line.toUtf8());
    this->fileHandle.flush();
}

QString LoggingChannel::generateDateString(const QDateTime &now)
{
    return now.toString("yyyy-MM-dd");
}

}  // namespace singletons
}  // namespace chatterino
