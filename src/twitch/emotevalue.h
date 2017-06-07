#pragma once

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

}  // namespace twitch
}  // namespace chatterino
