#pragma once

#include "channel.hpp"

namespace chatterino {

class ChannelManager
{
public:
    static ChannelManager &getInstance()
    {
        return instance;
    }

    std::shared_ptr<Channel> getWhispers();
    std::shared_ptr<Channel> getMentions();
    std::shared_ptr<Channel> getEmpty();

    const std::vector<std::shared_ptr<Channel>> getItems();

    std::shared_ptr<Channel> addChannel(const QString &channel);
    std::shared_ptr<Channel> getChannel(const QString &channel);
    void removeChannel(const QString &channel);

private:
    static ChannelManager instance;

    ChannelManager();

    QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> _channels;
    QMutex _channelsMutex;

    std::shared_ptr<Channel> _whispers;
    std::shared_ptr<Channel> _mentions;
    std::shared_ptr<Channel> _empty;
};

}  // namespace chatterino
