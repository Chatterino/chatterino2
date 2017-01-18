#ifndef TWITCHEMOTEVALUE_H
#define TWITCHEMOTEVALUE_H

#include "QString"

struct TwitchEmoteValue {
public:
    int
    getSet()
    {
        return set;
    }

    int
    getId()
    {
        return id;
    }

    const QString &
    getChannelName()
    {
        return channelName;
    }

private:
    int set;
    int id;
    QString channelName;
};

#endif  // TWITCHEMOTEVALUE_H
