#pragma once

#include "concurrentmap.h"
#include "logging/loggingchannel.h"
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

class ChannelManager;

typedef std::shared_ptr<Channel> SharedChannel;

class Channel
{
public:
    Channel(const QString &channel);

    boost::signals2::signal<void(messages::SharedMessage &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::SharedMessage &)> messageAppended;

    // properties
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBttvChannelEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getFfzChannelEmotes();

    bool isEmpty() const;
    const QString &getName() const;
    int getRoomID() const;
    const QString &getSubLink() const;
    const QString &getChannelLink() const;
    const QString &getPopoutPlayerLink() const;
    bool getIsLive() const;
    int getStreamViewerCount() const;
    const QString &getStreamStatus() const;
    const QString &getStreamGame() const;
    messages::LimitedQueueSnapshot<messages::SharedMessage> getMessageSnapshot();

    // methods
    void addMessage(messages::SharedMessage message);
    void reloadChannelEmotes();

    void sendMessage(const QString &message);

private:
    // variabeles
    messages::LimitedQueue<messages::SharedMessage> _messages;

    QString _name;
    int _roomID;

    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _ffzChannelEmotes;

    QString _subLink;
    QString _channelLink;
    QString _popoutPlayerLink;

    bool _isLive;
    int _streamViewerCount;
    QString _streamStatus;
    QString _streamGame;
    // std::shared_ptr<logging::Channel> _loggingChannel;

    // methods
    void reloadBttvEmotes();
    void reloadFfzEmotes();
};

}  // namespace chatterino
