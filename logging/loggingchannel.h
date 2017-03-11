#ifndef LOGGINGCHANNEL_H
#define LOGGINGCHANNEL_H

#include "messages/message.h"

#include <QDateTime>
#include <QFile>
#include <QString>

#include <memory>

namespace chatterino {
namespace logging {

class Channel
{
public:
    explicit Channel(const QString &_channelName,
                     const QString &_baseDirectory);
    ~Channel();

    void append(std::shared_ptr<messages::Message> message);

private:
    QString generateOpeningString(
        const QDateTime &now = QDateTime::currentDateTime()) const;
    QString generateClosingString(
        const QDateTime &now = QDateTime::currentDateTime()) const;

    void appendLine(const QString &line);

private:
    QString channelName;
    const QString &baseDirectory;
    QString fileName;
    QFile fileHandle;
};

}  // namespace logging
}  // namespace chatterino

#endif  // LOGGINGCHANNEL_H
