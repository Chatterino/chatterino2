#ifndef CHANNEL_H
#define CHANNEL_H

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <memory>

namespace chatterino {
namespace messages {
class Message;
}

class Channel
{
public:
    Channel(const QString &channel);

    // properties
    ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getBttvChannelEmotes()
    {
        return bttvChannelEmotes;
    }

    ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getFfzChannelEmotes()
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
    void addMessage(std::shared_ptr<messages::Message> message);

    QVector<std::shared_ptr<messages::Message>> getMessagesClone();

    QVector<std::shared_ptr<messages::Message>> &
    getMessages()
    {
        return messages;
    }

    void
    reloadChannelEmotes()
    {
        reloadBttvEmotes();
        reloadFfzEmotes();
    }

private:
    QVector<std::shared_ptr<messages::Message>> messages;

    QString name;
    int roomID;

    ConcurrentMap<QString, messages::LazyLoadedImage *> bttvChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> ffzChannelEmotes;
    QMutex messageMutex;

    QString subLink;
    QString channelLink;
    QString popoutPlayerLink;

    bool isLive;
    int streamViewerCount;
    QString streamStatus;
    QString streamGame;

    void reloadBttvEmotes();
    void reloadFfzEmotes();
};
}

#endif  // CHANNEL_H
