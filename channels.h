#ifndef CHANNELS_H
#define CHANNELS_H

#include "channel.h"

namespace chatterino {

class Channels
{
public:
    static Channel *
    getWhispers()
    {
        return &whispers;
    }

    static Channel *
    getMentions()
    {
        return &mentions;
    }

    static Channel *addChannel(const QString &channel);
    static Channel *getChannel(const QString &channel);
    static void removeChannel(const QString &channel);

private:
    Channels()
    {
    }

    static Channel whispers;
    static Channel mentions;
    static Channel empty;

    static QMap<QString, std::tuple<Channel *, int>> channels;
};
}
#endif  // CHANNELS_H
