#pragma once

#include "messages/Message.hpp"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <boost/noncopyable.hpp>

#include <memory>

namespace chatterino
{
    class Logging;

    class LoggingChannel : boost::noncopyable
    {
        explicit LoggingChannel(const QString& _channelName);

    public:
        ~LoggingChannel();
        void addMessage(MessagePtr message);

    private:
        void openLogFile();

        QString generateOpeningString(
            const QDateTime& now = QDateTime::currentDateTime()) const;
        QString generateClosingString(
            const QDateTime& now = QDateTime::currentDateTime()) const;

        void appendLine(const QString& line);

        QString generateDateString(const QDateTime& now);

        const QString channelName;
        QString baseDirectory;
        QString subDirectory;

        QFile fileHandle;

        QString dateString;

        friend class Logging;
    };

}  // namespace chatterino
