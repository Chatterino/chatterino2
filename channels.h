#ifndef CHANNELS_H
#define CHANNELS_H

#include "channel.h"

class Channels
{
public:
    static Channel *
    whispers()
    {
        return &m_whispers;
    }

    static Channel *
    mentions()
    {
        return &m_whispers;
    }

    static Channel *addChannel(const QString &channel);
    static Channel *getChannel(const QString &channel);
    static void removeChannel(const QString &channel);

private:
    Channels()
    {
    }

    static Channel m_whispers;
    static Channel m_mentions;
    static Channel m_empty;

    static QMap<QString, std::tuple<Channel *, int>> m_channels;
};

#endif  // CHANNELS_H
