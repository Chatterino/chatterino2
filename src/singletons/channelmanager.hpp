//#pragma once

//#include "channel.hpp"
//#include "channeldata.hpp"
//#include "providers/twitch/twitchchannel.hpp"

//#include <map>

// namespace chatterino {
// namespace singletons {
// class IrcManager;

// class ChannelManager
//{
//    explicit ChannelManager();

// public:
//    static ChannelManager &getInstance();

//    const std::vector<ChannelPtr> getItems();

//    const std::string &getUserID(const std::string &username);

//    void doOnAll(std::function<void(ChannelPtr)> func);

//    // Special channels
//    const ChannelPtr whispersChannel;
//    const ChannelPtr mentionsChannel;
//    const ChannelPtr emptyChannel;
//    void doOnAll(std::function<void(ChannelPtr)> func);
//    void doOnAllNormalChannels(std::function<void(ChannelPtr)> func);

// private:
//    std::map<std::string, std::string> usernameToID;
//    std::map<std::string, ChannelData> channelDatas;

//    QMutex channelsMutex;
//    QMap<QString, std::tuple<std::shared_ptr<TwitchChannel>, int>> twitchChannels;

//    pajlada::Signals::Signal<const QString &> ircJoin;
//    pajlada::Signals::Signal<const QString &> ircPart;

//    friend class singletons::IrcManager;
//};
//}  // namespace singletons
//}  // namespace chatterino
