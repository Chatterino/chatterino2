#include "LoggingChannel.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QDir>

#include <ctime>

namespace chatterino {

QByteArray endline("\n");

LoggingChannel::LoggingChannel(const QString &_channelName)
    : channelName(_channelName)
{
    if (this->channelName.startsWith("/whispers"))
    {
        this->subDirectory = "Whispers";
    }
    else if (channelName.startsWith("/mentions"))
    {
        this->subDirectory = "Mentions";
    }
    else
    {
        this->subDirectory =
            QStringLiteral("Channels") + QDir::separator() + channelName;
    }

    // FOURTF: change this when adding more providers
    this->subDirectory = "Twitch/" + this->subDirectory;

    getSettings()->logPath.connect([this](const QString &logPath, auto) {
        this->baseDirectory = logPath.isEmpty()
            ? getPaths()->messageLogDirectory
            : logPath;
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

    if (this->fileHandle.isOpen())
    {
        this->fileHandle.flush();
        this->fileHandle.close();
    }

    QString baseFileName = this->channelName + "-" + this->dateString + ".log";

    QString directory =
        this->baseDirectory + QDir::separator() + this->subDirectory;

    if (!QDir().mkpath(directory))
    {
        log("Unable to create logging path");
        return;
    }

    // Open file handle to log file of current date
    QString fileName = directory + QDir::separator() + baseFileName;
    log("Logging to {}", fileName);
    this->fileHandle.setFileName(fileName);

    this->fileHandle.open(QIODevice::Append);

    this->appendLine(this->generateOpeningString(now));
}

void LoggingChannel::addMessage(MessagePtr message)
{
    QDateTime now = QDateTime::currentDateTime();

    QString messageDateString = this->generateDateString(now);
    if (messageDateString != this->dateString)
    {
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

}  // namespace chatterino
