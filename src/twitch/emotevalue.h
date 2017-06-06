#ifndef TWITCHEMOTEVALUE_H
#define TWITCHEMOTEVALUE_H

#include "QString"

namespace chatterino {
namespace twitch {

struct EmoteValue {
public:
    int getSet()
    {
        return _set;
    }

    int getId()
    {
        return _id;
    }

    const QString &getChannelName()
    {
        return _channelName;
    }

private:
    int _set;
    int _id;
    QString _channelName;
};
}
}

#endif  // TWITCHEMOTEVALUE_H
