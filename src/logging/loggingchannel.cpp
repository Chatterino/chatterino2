#include "loggingchannel.hpp"
#include "loggingmanager.hpp"

#include <QDir>

#include <ctime>

namespace chatterino {
namespace logging {

Channel::Channel(const QString &_channelName, const QString &_baseDirectory)
    : channelName(_channelName)
    , baseDirectory(_baseDirectory)
{
    QDateTime now = QDateTime::currentDateTime();

    this->fileName = this->channelName + "-" + now.toString("yyyy-MM-dd") + ".log";

    // Open file handle to log file of current date
    this->fileHandle.setFileName(this->baseDirectory + QDir::separator() + this->fileName);

    this->fileHandle.open(QIODevice::Append);
    this->appendLine(this->generateOpeningString(now));
}

Channel::~Channel()
{
    this->appendLine(this->generateClosingString());
    this->fileHandle.close();
}

void Channel::append(std::shared_ptr<messages::Message> message)
{
    QDateTime now = QDateTime::currentDateTime();

    QString str;
    str.append('[');
    str.append(now.toString("HH:mm:ss"));
    str.append("] ");
    str.append(message->loginName);
    str.append(": ");
    str.append(message->getContent());
    str.append('\n');
    this->appendLine(str);
}

QString Channel::generateOpeningString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Start logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss "));
    ret.append(now.timeZoneAbbreviation());
    ret.append('\n');

    return ret;
}

QString Channel::generateClosingString(const QDateTime &now) const
{
    QString ret = QLatin1Literal("# Stop logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss"));
    ret.append(now.timeZoneAbbreviation());
    ret.append('\n');

    return ret;
}

void Channel::appendLine(const QString &line)
{
    this->fileHandle.write(line.toUtf8());
    this->fileHandle.flush();
}

}  // namespace logging
}  // namespace chatterino
