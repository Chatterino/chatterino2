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
public:
    explicit ChannelManager(WindowManager &_windowManager, EmoteManager &_emoteManager,
                            IrcManager &_ircManager);

    std::shared_ptr<Channel> getWhispers();
    std::shared_ptr<Channel> getMentions();
    std::shared_ptr<Channel> getEmpty();

    const std::vector<std::shared_ptr<Channel>> getItems();

    std::shared_ptr<Channel> addChannel(const QString &channel);
    std::shared_ptr<Channel> getChannel(const QString &channel);
    void removeChannel(const QString &channel);

    const std::string &getUserID(const std::string &username);

private:
    WindowManager &windowManager;
    EmoteManager &emoteManager;
    IrcManager &ircManager;

    std::map<std::string, std::string> usernameToID;
    std::map<std::string, ChannelData> channelDatas;

    QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> _channels;
    QMutex _channelsMutex;

    std::shared_ptr<Channel> _whispers;
    std::shared_ptr<Channel> _mentions;
    std::shared_ptr<Channel> _empty;
};

}  // namespace chatterino
