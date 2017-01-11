#ifndef TWITCHEMOTEVALUE_H
#define TWITCHEMOTEVALUE_H

#include "QString"

struct TwitchEmoteValue {
public:
    int
    set()
    {
        return m_set;
    }

    int
    id()
    {
        return m_id;
    }

    const QString &
    channelName()
    {
        return m_channelName;
    }

private:
    int m_set;
    int m_id;
    QString m_channelName;
};

#endif  // TWITCHEMOTEVALUE_H
