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

    const std::vector<SharedChannel> getItems();

    SharedChannel addTwitchChannel(const QString &channel);
    SharedChannel getTwitchChannel(const QString &channel);
    void removeTwitchChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);

    void doOnAll(std::function<void(SharedChannel)> func);

    // Special channels
    const SharedChannel whispersChannel;
    const SharedChannel mentionsChannel;
    const SharedChannel emptyChannel;

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
