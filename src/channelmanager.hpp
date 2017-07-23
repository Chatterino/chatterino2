#pragma once

#include "channel.hpp"
#include "channeldata.hpp"

#include <map>

namespace chatterino {

class WindowManager;
class EmoteManager;
class IrcManager;

class ChannelManager
{
    WindowManager &windowManager;
    EmoteManager &emoteManager;
    IrcManager &ircManager;

public:
    explicit ChannelManager(WindowManager &_windowManager, EmoteManager &_emoteManager,
                            IrcManager &_ircManager);

    const std::vector<std::shared_ptr<Channel>> getItems();

    std::shared_ptr<Channel> addChannel(const QString &channel);
    std::shared_ptr<Channel> getChannel(const QString &channel);
    void removeChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);

    // Special channels
    const std::shared_ptr<Channel> whispersChannel;
    const std::shared_ptr<Channel> mentionsChannel;
    const std::shared_ptr<Channel> emptyChannel;

private:
    std::map<std::string, std::string> usernameToID;
    std::map<std::string, ChannelData> channelDatas;

    QMutex channelsMutex;
    QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> channels;
};

}  // namespace chatterino
