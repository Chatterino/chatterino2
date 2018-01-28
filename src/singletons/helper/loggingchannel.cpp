#include "loggingchannel.hpp"

#include <QDir>

#include <ctime>

namespace chatterino {
namespace singletons {

LoggingChannel::LoggingChannel(const QString &_channelName, const QString &_baseDirectory)
    : channelName(_channelName)
    , baseDirectory(_baseDirectory)
{
    QDateTime now = QDateTime::currentDateTime();

    QString baseFileName = this->channelName + "-" + now.toString("yyyy-MM-dd") + ".log";

    // Open file handle to log file of current date
    this->fileHandle.setFileName(this->baseDirectory + QDir::separator() + baseFileName);

    this->fileHandle.open(QIODevice::Append);
    this->appendLine(this->generateOpeningString(now));
}

LoggingChannel::~LoggingChannel()
{
    this->appendLine(this->generateClosingString());
    this->fileHandle.close();
}

void LoggingChannel::addMessage(std::shared_ptr<messages::Message> message)
{
    QDateTime now = QDateTime::currentDateTime();

    QString str;
    str.append('[');
    str.append(now.toString("HH:mm:ss"));
    str.append("] ");

    if ((message->flags & messages::Message::MessageFlags::System) != 0) {
        str.append(message->searchText);
        str.append('\n');
    } else {
        str.append(message->loginName);
        str.append(": ");
        str.append(message->searchText);
        str.append('\n');
    }

    this->appendLine(str);
}

QString LoggingChannel::generateOpeningString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Start logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss "));
    ret.append(now.timeZoneAbbreviation());
    ret.append('\n');

    return ret;
}

QString LoggingChannel::generateClosingString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Stop logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss"));
    ret.append(now.timeZoneAbbreviation());
    ret.append('\n');

    return ret;
}

void LoggingChannel::appendLine(const QString &line)
{
    this->fileHandle.write(line.toUtf8());
    this->fileHandle.flush();
}

}  // namespace singletons
}  // namespace chatterino
