#pragma once

#include "channel.hpp"
#include "channeldata.hpp"
#include "twitch/twitchchannel.hpp"

#include <map>

namespace chatterino {

class WindowManager;
class IrcManager;

class ChannelManager
{
    explicit ChannelManager();

public:
    static ChannelManager &getInstance();

    const std::vector<std::shared_ptr<Channel>> getItems();

    std::shared_ptr<Channel> addTwitchChannel(const QString &channel);
    std::shared_ptr<Channel> getTwitchChannel(const QString &channel);
    void removeTwitchChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);

    void doOnAll(std::function<void(std::shared_ptr<Channel>)> func);

    // Special channels
    const std::shared_ptr<Channel> whispersChannel;
    const std::shared_ptr<Channel> mentionsChannel;
    const std::shared_ptr<Channel> emptyChannel;

private:
    std::map<std::string, std::string> usernameToID;
    std::map<std::string, ChannelData> channelDatas;

    QMutex channelsMutex;
    QMap<QString, std::tuple<std::shared_ptr<twitch::TwitchChannel>, int>> twitchChannels;

    pajlada::Signals::Signal<const QString &> ircJoin;
    pajlada::Signals::Signal<const QString &> ircPart;

    friend class IrcManager;
};

}  // namespace chatterino
