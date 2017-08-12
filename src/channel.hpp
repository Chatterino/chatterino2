#pragma once

#include "concurrentmap.hpp"
#include "emotemanager.hpp"
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
class IrcManager;

class Channel
{
    WindowManager &windowManager;
    EmoteManager &emoteManager;
    IrcManager &ircManager;

public:
    explicit Channel(WindowManager &_windowManager, EmoteManager &_emoteManager,
                     IrcManager &_ircManager, const QString &channelName, bool isSpecial = false);

    boost::signals2::signal<void(messages::SharedMessage &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::SharedMessage &)> messageAppended;

    bool isEmpty() const;
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
    const QString name;

private:
    // variables
    messages::LimitedQueue<messages::SharedMessage> _messages;

public:
    const EmoteManager::EmoteMap &bttvChannelEmotes;
    const EmoteManager::EmoteMap &ffzChannelEmotes;

private:
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
