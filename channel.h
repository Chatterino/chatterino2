#ifndef CHANNEL_H
#define CHANNEL_H

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"
#include "messages/limitedqueue.h"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <boost/signals2.hpp>
#include <memory>

namespace chatterino {
namespace messages {
class Message;
}

class Channel
{
public:
    Channel(const QString &channel);

    boost::signals2::signal<void(std::shared_ptr<messages::Message> &)>
        messageRemovedFromStart;
    boost::signals2::signal<void(std::shared_ptr<messages::Message> &)>
        messageAppended;

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

    bool
    isEmpty() const
    {
        return name.isEmpty();
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
    messages::LimitedQueueSnapshot<std::shared_ptr<messages::Message>>
    getMessageSnapshot()
    {
        return messages.getSnapshot();
    }

    void addMessage(std::shared_ptr<messages::Message> message);

    void
    reloadChannelEmotes()
    {
        this->reloadBttvEmotes();
        this->reloadFfzEmotes();
    }

private:
    messages::LimitedQueue<std::shared_ptr<messages::Message>> messages;

    QString name;
    int roomID;

    ConcurrentMap<QString, messages::LazyLoadedImage *> bttvChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> ffzChannelEmotes;

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
