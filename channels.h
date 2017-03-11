#ifndef CHANNELS_H
#define CHANNELS_H

#include "channel.h"

namespace chatterino {

void initChannels();

class Channels
{
public:
    static std::shared_ptr<Channel> getWhispers();
    static std::shared_ptr<Channel> getMentions();
    static std::shared_ptr<Channel> getEmpty();

    const static std::vector<std::shared_ptr<Channel>> getItems();

    static std::shared_ptr<Channel> addChannel(const QString &channel);
    static std::shared_ptr<Channel> getChannel(const QString &channel);
    static void removeChannel(const QString &channel);

private:
    Channels()
    {
    }

    static QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> channels;
    static QMutex channelsMutex;
};

}  // namespace chatterino

#endif  // CHANNELS_H
