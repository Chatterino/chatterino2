//#include "singletons/channelmanager.hpp"
//#include "singletons/ircmanager.hpp"

// namespace chatterino {
// namespace singletons {

// ChannelManager &ChannelManager::getInstance()
//{
//    static ChannelManager instance;
//    return instance;
//}

// ChannelManager::ChannelManager()
//    : whispersChannel(new Channel("/whispers"))
//    , mentionsChannel(new Channel("/mentions"))
//    , emptyChannel(new Channel(""))
//{
//}

// const std::vector<ChannelPtr> ChannelManager::getItems()
//{
//    QMutexLocker locker(&this->channelsMutex);

//    std::vector<ChannelPtr> items;

//    for (auto &item : this->twitchChannels.values()) {
//        items.push_back(std::get<0>(item));
//    }

//    return items;
//}

// ChannelPtr ChannelManager::addTwitchChannel(const QString &rawChannelName)
//{
//    QString channelName = rawChannelName.toLower();

//    if (channelName.length() > 1 && channelName.at(0) == '/') {
//        return this->getTwitchChannel(channelName);
//    }

//    if (channelName.length() > 0 && channelName.at(0) == '#') {
//        channelName = channelName.mid(1);
//    }

//    QMutexLocker locker(&this->channelsMutex);

//    auto it = this->twitchChannels.find(channelName);

//    if (it == this->twitchChannels.end()) {
//        auto channel = std::make_shared<TwitchChannel>(channelName);

//        this->twitchChannels.insert(channelName, std::make_tuple(channel, 1));

//        this->ircJoin.invoke(channelName);

//        return channel;
//    }

//    std::get<1>(it.value())++;

//    return std::get<0>(it.value());
//}

// ChannelPtr ChannelManager::getTwitchChannel(const QString &channel)
//{
//    QMutexLocker locker(&this->channelsMutex);

//    QString c = channel.toLower();

//    if (channel.length() > 1 && channel.at(0) == '/') {
//        if (c == "/whispers") {
//            return whispersChannel;
//        }

//        if (c == "/mentions") {
//            return mentionsChannel;
//        }

//        return emptyChannel;
//    }

//    auto a = this->twitchChannels.find(c);

//    if (a == this->twitchChannels.end()) {
//        return emptyChannel;
//    }

//    return std::get<0>(a.value());
//}

// void ChannelManager::removeTwitchChannel(const QString &channel)
//{
//    QMutexLocker locker(&this->channelsMutex);

//    if (channel.length() > 1 && channel.at(0) == '/') {
//        return;
//    }

//    QString c = channel.toLower();

//    auto a = this->twitchChannels.find(c);

//    if (a == this->twitchChannels.end()) {
//        return;
//    }

//    std::get<1>(a.value())--;

//    if (std::get<1>(a.value()) == 0) {
//        this->ircPart.invoke(c);
//        this->twitchChannels.remove(c);
//    }
//}

// const std::string &ChannelManager::getUserID(const std::string &username)
//{
//    /* TODO: Implement
//    auto it = this->usernameToID.find(username);

//    if (it != std::end(this->usernameToID)) {
//        return *it;
//    }
//    */

//    static std::string temporary = "xd";
//    return temporary;
//}

// void ChannelManager::doOnAll(std::function<void(ChannelPtr)> func)
//{
//    for (const auto &channel : this->twitchChannels) {
//        func(std::get<0>(channel));
//    }

//    func(this->whispersChannel);
//    func(this->mentionsChannel);
//}

//}  // namespace singletons
//}  // namespace chatterino
