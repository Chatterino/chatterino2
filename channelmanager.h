#ifndef CHANNELS_H
#define CHANNELS_H

#include "channel.h"

namespace  chatterino {

class ChannelManager
{
public:
    static ChannelManager &getInstance()
    {
        return instance;
    }

    SharedChannel getWhispers();
    SharedChannel getMentions();
    SharedChannel getEmpty();

    const std::vector<SharedChannel> getItems();

    SharedChannel addChannel(const QString &channel);
    SharedChannel getChannel(const QString &channel);
    void removeChannel(const QString &channel);

private:
    static ChannelManager instance;

    ChannelManager();

    QMap<QString, std::tuple<SharedChannel, int>> _channels;
    QMutex _channelsMutex;

    SharedChannel _whispers;
    SharedChannel _mentions;
    SharedChannel _empty;
};

}  // namespace  chatterino

#endif  // CHANNELS_H
