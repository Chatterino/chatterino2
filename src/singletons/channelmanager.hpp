#pragma once

#include "channel.hpp"
#include "channeldata.hpp"
#include "twitch/twitchchannel.hpp"

#include <map>

namespace chatterino {
namespace singletons {
class IrcManager;

class ChannelManager
{
    explicit ChannelManager();

public:
    static ChannelManager &getInstance();

    const std::vector<ChannelPtr> getItems();

    ChannelPtr addTwitchChannel(const QString &channel);
    ChannelPtr getTwitchChannel(const QString &channel);
    void removeTwitchChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);

    void doOnAll(std::function<void(ChannelPtr)> func);

    // Special channels
    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    const ChannelPtr emptyChannel;

private:
    std::map<std::string, std::string> usernameToID;
    std::map<std::string, ChannelData> channelDatas;

    QMutex channelsMutex;
    QMap<QString, std::tuple<std::shared_ptr<twitch::TwitchChannel>, int>> twitchChannels;

    pajlada::Signals::Signal<const QString &> ircJoin;
    pajlada::Signals::Signal<const QString &> ircPart;

    friend class singletons::IrcManager;
};

}  // namespace chatterino
}
