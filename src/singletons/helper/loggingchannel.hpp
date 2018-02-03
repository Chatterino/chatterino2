#pragma once

#include "messages/message.hpp"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <boost/noncopyable.hpp>

#include <memory>

namespace chatterino {
namespace singletons {

class LoggingChannel : boost::noncopyable
{
    explicit LoggingChannel(const QString &_channelName, const QString &_baseDirectory);

public:
    ~LoggingChannel();
    void addMessage(std::shared_ptr<messages::Message> message);

private:
    void openLogFile();

    QString generateOpeningString(const QDateTime &now = QDateTime::currentDateTime()) const;
    QString generateClosingString(const QDateTime &now = QDateTime::currentDateTime()) const;

    void appendLine(const QString &line);

    QString generateDateString(const QDateTime &now);

    const QString channelName;
    const QString baseDirectory;

    QFile fileHandle;

    QString dateString;

    friend class LoggingManager;
};

}  // namespace singletons
}  // namespace chatterino
