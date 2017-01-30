#ifndef CHANNELS_H
#define CHANNELS_H

#include "channel.h"

namespace chatterino {

class Channels
{
public:
    static std::shared_ptr<Channel>
    getWhispers()
    {
        return whispers;
    }

    static std::shared_ptr<Channel>
    getMentions()
    {
        return mentions;
    }

    static std::shared_ptr<Channel>
    getEmpty()
    {
        return empty;
    }

    const static std::vector<std::shared_ptr<Channel>> getItems();

    static std::shared_ptr<Channel> addChannel(const QString &channel);
    static std::shared_ptr<Channel> getChannel(const QString &channel);
    static void removeChannel(const QString &channel);

private:
    Channels()
    {
    }

    static std::shared_ptr<Channel> whispers;
    static std::shared_ptr<Channel> mentions;
    static std::shared_ptr<Channel> empty;

    static QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> channels;
    static QMutex channelsMutex;
};
}
#endif  // CHANNELS_H
