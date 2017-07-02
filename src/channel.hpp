#pragma once

#include "concurrentmap.hpp"
#include "logging/loggingchannel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueue.hpp"

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

class WindowManager;
class EmoteManager;
class IrcManager;

class Channel
{
public:
    explicit Channel(WindowManager &_windowManager, EmoteManager &_emoteManager,
                     IrcManager &_ircManager, const QString &channel, bool isSpecial = false);

    boost::signals2::signal<void(messages::SharedMessage &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::SharedMessage &)> messageAppended;

    // properties
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBttvChannelEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getFfzChannelEmotes();

    bool isEmpty() const;
    const QString &getName() const;
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

    std::string roomID;

private:
    WindowManager &windowManager;
    EmoteManager &emoteManager;
    IrcManager &ircManager;

    // variabeles
    messages::LimitedQueue<messages::SharedMessage> _messages;

    QString _name;

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
};

}  // namespace chatterino
