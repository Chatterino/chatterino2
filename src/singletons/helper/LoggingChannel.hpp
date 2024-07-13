#pragma once

#include <QDateTime>
#include <QFile>
#include <QString>

#include <memory>

namespace chatterino {

class Logging;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;

class LoggingChannel
{
    explicit LoggingChannel(const QString &_channelName,
                            const QString &platform);

public:
    ~LoggingChannel();

    LoggingChannel(const LoggingChannel &) = delete;
    LoggingChannel &operator=(const LoggingChannel &) = delete;

    LoggingChannel(LoggingChannel &&) = delete;
    LoggingChannel &operator=(LoggingChannel &&) = delete;

    void addMessage(MessagePtr message, const QString &streamID);

private:
    void openLogFile();
    void openStreamLogFile(const QString &streamID);

    QString generateClosingString(
        const QDateTime &now = QDateTime::currentDateTime()) const;

    QString generateDateString(const QDateTime &now);

    const QString channelName;
    const QString platform;
    QString baseDirectory;
    QString subDirectory;

    QFile fileHandle;
    QFile currentStreamFileHandle;
    QString currentStreamID;

    QString dateString;

    friend class Logging;
};

}  // namespace chatterino
