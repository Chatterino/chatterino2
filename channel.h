#ifndef CHANNEL_H
#define CHANNEL_H

#include "concurrentmap.h"
#include "lazyloadedimage.h"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <memory>

class Message;

class Channel
{
    // static

public:
    static Channel whispers;
    static Channel mentions;

    static Channel *addChannel(const QString &channel);
    static Channel *getChannel(const QString &channel);
    static void removeChannel(const QString &channel);

private:
    static QMap<QString, Channel *> channels;

    // members

public:
    // properties
    const ConcurrentMap<QString, LazyLoadedImage *> &
    bttvChannelEmotes() const
    {
        return m_bttvChannelEmotes;
    }
    const ConcurrentMap<QString, LazyLoadedImage *> &
    ffzChannelEmotes() const
    {
        return m_ffzChannelEmotes;
    }

    const QMutex &
    messageMutex() const
    {
        return m_messageMutex;
    }

    const QString &
    name() const
    {
        return m_name;
    }
    int
    roomID() const
    {
        return m_roomID;
    }

    const QString &
    subLink() const
    {
        return m_subLink;
    }
    const QString &
    channelLink() const
    {
        return m_channelLink;
    }
    const QString &
    popoutPlayerLink() const
    {
        return m_popoutPlayerLink;
    }

    bool
    isLive() const
    {
        return m_isLive;
    }
    int
    streamViewerCount() const
    {
        return m_streamViewerCount;
    }
    const QString &
    streamStatus() const
    {
        return m_streamStatus;
    }
    const QString &
    streamGame() const
    {
        return m_streamGame;
    }

    // methods
    void addMessage(std::shared_ptr<Message> message);

    QVector<std::shared_ptr<Message>> getMessagesClone();

private:
    Channel(QString channel);

    ConcurrentMap<QString, LazyLoadedImage *> m_bttvChannelEmotes;
    ConcurrentMap<QString, LazyLoadedImage *> m_ffzChannelEmotes;
    QMutex m_messageMutex;

    int m_referenceCount = 0;

    QVector<std::shared_ptr<Message>> m_messages;

    QString m_name;
    int m_roomID;

    QString m_subLink;
    QString m_channelLink;
    QString m_popoutPlayerLink;

    bool m_isLive;
    int m_streamViewerCount;
    QString m_streamStatus;
    QString m_streamGame;
};

#endif  // CHANNEL_H
