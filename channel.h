#ifndef CHANNEL_H
#define CHANNEL_H

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <memory>

using namespace chatterino::messages;

namespace chatterino {

namespace messages {
class Message;
}

class Channel
{
public:
    Channel(const QString &channel);

    // properties
    const ConcurrentMap<QString, LazyLoadedImage *> &
    getBttvChannelEmotes() const
    {
        return bttvChannelEmotes;
    }

    const ConcurrentMap<QString, LazyLoadedImage *> &
    getFfzChannelEmotes() const
    {
        return ffzChannelEmotes;
    }

    const QMutex &
    getMessageMutex() const
    {
        return messageMutex;
    }

    const QString &
    getName() const
    {
        return name;
    }

    int
    getRoomID() const
    {
        return roomID;
    }

    const QString &
    getSubLink() const
    {
        return subLink;
    }
    const QString &
    getChannelLink() const
    {
        return channelLink;
    }
    const QString &
    getPopoutPlayerLink() const
    {
        return popoutPlayerLink;
    }

    bool
    getIsLive() const
    {
        return isLive;
    }
    int
    getStreamViewerCount() const
    {
        return streamViewerCount;
    }
    const QString &
    getStreamStatus() const
    {
        return streamStatus;
    }
    const QString &
    getStreamGame() const
    {
        return streamGame;
    }

    // methods
    void addMessage(std::shared_ptr<Message> message);

    QVector<std::shared_ptr<Message>> getMessagesClone();

private:
    QVector<std::shared_ptr<Message>> messages;

    QString name;
    int roomID;

    ConcurrentMap<QString, LazyLoadedImage *> bttvChannelEmotes;
    ConcurrentMap<QString, LazyLoadedImage *> ffzChannelEmotes;
    QMutex messageMutex;

    QString subLink;
    QString channelLink;
    QString popoutPlayerLink;

    bool isLive;
    int streamViewerCount;
    QString streamStatus;
    QString streamGame;
};
}

#endif  // CHANNEL_H
