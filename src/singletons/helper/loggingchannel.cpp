#include "loggingchannel.hpp"

#include <QDir>

#include <ctime>

namespace chatterino {
namespace singletons {

QByteArray endline("\n");

LoggingChannel::LoggingChannel(const QString &_channelName, const QString &_baseDirectory)
    : channelName(_channelName)
    , baseDirectory(_baseDirectory)
{
    QDateTime now = QDateTime::currentDateTime();

    this->dateString = this->generateDateString(now);

    this->openLogFile();

    this->appendLine(this->generateOpeningString(now));
}

LoggingChannel::~LoggingChannel()
{
    this->appendLine(this->generateClosingString());
    this->fileHandle.close();
}

void LoggingChannel::openLogFile()
{
    if (this->fileHandle.isOpen()) {
        this->fileHandle.flush();
        this->fileHandle.close();
    }

    QString baseFileName = this->channelName + "-" + this->dateString + ".log";

    // Open file handle to log file of current date
    this->fileHandle.setFileName(this->baseDirectory + QDir::separator() + baseFileName);

    this->fileHandle.open(QIODevice::Append);
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
    /*
    auto a1 = line.toUtf8();
    auto a2 = line.toLatin1();
    auto a3 = line.toLocal8Bit();

    auto a4 = line.data();

    auto a5 = line.toStdString();
    */

    // this->fileHandle.write(a5.c_str(), a5.length());
    // this->fileHandle.write(a5.c_str(), a5.length());
    this->fileHandle.write(line.toUtf8());
    this->fileHandle.flush();
}

QString LoggingChannel::generateDateString(const QDateTime &now)
{
    return now.toString("yyyy-MM-dd");
}

}  // namespace singletons
}  // namespace chatterino
