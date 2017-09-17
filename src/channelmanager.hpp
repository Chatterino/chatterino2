#pragma once

#include "channel.hpp"
#include "channeldata.hpp"
#include "twitch/twitchchannel.hpp"

#include <map>

namespace chatterino {

class WindowManager;
class EmoteManager;
class IrcManager;

class ChannelManager
{
public:
    explicit ChannelManager(WindowManager &_windowManager, EmoteManager &_emoteManager,
                            IrcManager &_ircManager);

    WindowManager &windowManager;
    EmoteManager &emoteManager;
    IrcManager &ircManager;

    const std::vector<std::shared_ptr<Channel>> getItems();

    std::shared_ptr<twitch::TwitchChannel> addTwitchChannel(const QString &channel);
    std::shared_ptr<twitch::TwitchChannel> getTwitchChannel(const QString &channel);
    void removeTwitchChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);
    EmoteManager &getEmoteManager();
    WindowManager &getWindowManager();

    // Special channels
    const std::shared_ptr<twitch::TwitchChannel> whispersChannel;
    const std::shared_ptr<twitch::TwitchChannel> mentionsChannel;
    const std::shared_ptr<twitch::TwitchChannel> emptyChannel;

private:
    std::map<std::string, std::string> usernameToID;
    std::map<std::string, ChannelData> channelDatas;

    QMutex channelsMutex;
    QMap<QString, std::tuple<std::shared_ptr<twitch::TwitchChannel>, int>> twitchChannels;
};

}  // namespace chatterino
