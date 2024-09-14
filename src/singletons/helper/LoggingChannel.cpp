#include "singletons/helper/LoggingChannel.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "messages/Message.hpp"
#include "messages/MessageThread.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QDateTime>
#include <QDir>

namespace {

const QByteArray ENDLINE("\n");

void appendLine(QFile &fileHandle, const QString &line)
{
    assert(fileHandle.isOpen());
    assert(fileHandle.isWritable());

    fileHandle.write(line.toUtf8());
    fileHandle.flush();
}

QString generateOpeningString(
    const QDateTime &now = QDateTime::currentDateTime())
{
    QString ret("# Start logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss "));
    ret.append(now.timeZoneAbbreviation());
    ret.append(ENDLINE);

    return ret;
}

QString generateClosingString(
    const QDateTime &now = QDateTime::currentDateTime())
{
    QString ret("# Stop logging at ");

    ret.append(now.toString("yyyy-MM-dd HH:mm:ss "));
    ret.append(now.timeZoneAbbreviation());
    ret.append(ENDLINE);

    return ret;
}

QString generateDateString(const QDateTime &now)
{
    return now.toString("yyyy-MM-dd");
}

}  // namespace

namespace chatterino {

LoggingChannel::LoggingChannel(QString _channelName, QString _platform)
    : channelName(std::move(_channelName))
    , platform(std::move(_platform))
{
    if (this->channelName.startsWith("/whispers"))
    {
        this->subDirectory = "Whispers";
    }
    else if (channelName.startsWith("/mentions"))
    {
        this->subDirectory = "Mentions";
    }
    else if (channelName.startsWith("/live"))
    {
        this->subDirectory = "Live";
    }
    else if (channelName.startsWith("/automod"))
    {
        this->subDirectory = "AutoMod";
    }
    else
    {
        this->subDirectory =
            QStringLiteral("Channels") + QDir::separator() + channelName;
    }

    // enforce capitalized platform names
    this->subDirectory = platform[0].toUpper() + platform.mid(1).toLower() +
                         QDir::separator() + this->subDirectory;

    getSettings()->logPath.connect([this](const QString &logPath, auto) {
        this->baseDirectory = logPath.isEmpty()
                                  ? getApp()->getPaths().messageLogDirectory
                                  : logPath;
        this->openLogFile();
    });
}

LoggingChannel::~LoggingChannel()
{
    appendLine(this->fileHandle, generateClosingString());
    this->fileHandle.close();
    this->currentStreamFileHandle.close();
}

void LoggingChannel::openLogFile()
{
    QDateTime now = QDateTime::currentDateTime();
    this->dateString = generateDateString(now);

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
        qCDebug(chatterinoHelper) << "Unable to create logging path";
        return;
    }

    // Open file handle to log file of current date
    QString fileName = directory + QDir::separator() + baseFileName;
    qCDebug(chatterinoHelper) << "Logging to" << fileName;
    this->fileHandle.setFileName(fileName);

    this->fileHandle.open(QIODevice::Append);

    appendLine(this->fileHandle, generateOpeningString(now));
}

void LoggingChannel::openStreamLogFile(const QString &streamID)
{
    QDateTime now = QDateTime::currentDateTime();
    this->currentStreamID = streamID;

    if (this->currentStreamFileHandle.isOpen())
    {
        this->currentStreamFileHandle.flush();
        this->currentStreamFileHandle.close();
    }

    QString baseFileName = this->channelName + "-" + streamID + ".log";

    QString directory =
        this->baseDirectory + QDir::separator() + this->subDirectory;

    if (!QDir().mkpath(directory))
    {
        qCDebug(chatterinoHelper) << "Unable to create logging path";
        return;
    }

    QString fileName = directory + QDir::separator() + baseFileName;
    qCDebug(chatterinoHelper) << "Logging stream to" << fileName;
    this->currentStreamFileHandle.setFileName(fileName);

    this->currentStreamFileHandle.open(QIODevice::Append);
    appendLine(this->currentStreamFileHandle, generateOpeningString(now));
}

void LoggingChannel::addMessage(const MessagePtr &message,
                                const QString &streamID)
{
    QDateTime now = QDateTime::currentDateTime();

    QString messageDateString = generateDateString(now);
    if (messageDateString != this->dateString)
    {
        this->dateString = messageDateString;
        this->openLogFile();
    }

    QString str;
    if (channelName.startsWith("/mentions") ||
        channelName.startsWith("/automod"))
    {
        str.append("#" + message->channelName + " ");
    }

    str.append('[');
    str.append(now.toString("HH:mm:ss"));
    str.append("] ");

    QString messageText;
    if (message->loginName.isEmpty())
    {
        // This accounts for any messages not explicitly sent by a user, like
        // system messages, parts of announcements, subs etc.
        messageText = message->messageText;
    }
    else
    {
        if (message->localizedName.isEmpty())
        {
            messageText = message->loginName + ": " + message->messageText;
        }
        else
        {
            messageText = message->localizedName + " " + message->loginName +
                          ": " + message->messageText;
        }
    }

    if ((message->flags.has(MessageFlag::ReplyMessage) &&
         getSettings()->stripReplyMention) &&
        !getSettings()->hideReplyContext)
    {
        qsizetype colonIndex = messageText.indexOf(':');
        if (colonIndex != -1)
        {
            QString rootMessageChatter;
            if (message->replyParent)
            {
                rootMessageChatter = message->replyParent->loginName;
            }
            else
            {
                // we actually want to use 'reply-parent-user-login' tag here,
                // but it's not worth storing just for this edge case
                rootMessageChatter = message->replyThread->root()->loginName;
            }
            messageText.insert(colonIndex + 1, " @" + rootMessageChatter);
        }
    }
    str.append(messageText);
    str.append(ENDLINE);

    appendLine(this->fileHandle, str);

    if (!streamID.isEmpty() && getSettings()->separatelyStoreStreamLogs)
    {
        if (this->currentStreamID != streamID)
        {
            this->openStreamLogFile(streamID);
        }

        appendLine(this->currentStreamFileHandle, str);
    }
}

}  // namespace chatterino
