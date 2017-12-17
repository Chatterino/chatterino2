#include "channelmanager.hpp"
#include "ircmanager.hpp"

using namespace chatterino::twitch;

namespace chatterino {

ChannelManager::ChannelManager(WindowManager &_windowManager, IrcManager &_ircManager)
    : windowManager(_windowManager)
    , ircManager(_ircManager)
    , whispersChannel(new TwitchChannel(_ircManager, "/whispers", true))
    , mentionsChannel(new TwitchChannel(_ircManager, "/mentions", true))
    , emptyChannel(new TwitchChannel(_ircManager, "", true))
{
}

const std::vector<std::shared_ptr<Channel>> ChannelManager::getItems()
{
    QMutexLocker locker(&this->channelsMutex);

    std::vector<std::shared_ptr<Channel>> items;

    for (auto &item : this->twitchChannels.values()) {
        items.push_back(std::get<0>(item));
    }

    return items;
}

std::shared_ptr<TwitchChannel> ChannelManager::addTwitchChannel(const QString &rawChannelName)
{
    QString channelName = rawChannelName.toLower();

    if (channelName.length() > 1 && channelName.at(0) == '/') {
        return this->getTwitchChannel(channelName);
    }

    if (channelName.length() > 0 && channelName.at(0) == '#') {
        channelName = channelName.mid(1);
    }

    QMutexLocker locker(&this->channelsMutex);

    auto it = this->twitchChannels.find(channelName);

    if (it == this->twitchChannels.end()) {
        auto channel = std::make_shared<TwitchChannel>(this->ircManager, channelName);

        this->twitchChannels.insert(channelName, std::make_tuple(channel, 1));

        this->ircManager.joinChannel(channelName);

        return channel;
    }

    std::get<1>(it.value())++;

    return std::get<0>(it.value());
}

std::shared_ptr<TwitchChannel> ChannelManager::getTwitchChannel(const QString &channel)
{
    QMutexLocker locker(&this->channelsMutex);

    QString c = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        if (c == "/whispers") {
            return whispersChannel;
        }

        if (c == "/mentions") {
            return mentionsChannel;
        }

        return emptyChannel;
    }

    auto a = this->twitchChannels.find(c);

    if (a == this->twitchChannels.end()) {
        return emptyChannel;
    }

    return std::get<0>(a.value());
}

void ChannelManager::removeTwitchChannel(const QString &channel)
{
    QMutexLocker locker(&this->channelsMutex);

    if (channel.length() > 1 && channel.at(0) == '/') {
        return;
    }

    QString c = channel.toLower();

    auto a = this->twitchChannels.find(c);

    if (a == this->twitchChannels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        this->ircManager.partChannel(c);
        this->twitchChannels.remove(c);
    }
}

const std::string &ChannelManager::getUserID(const std::string &username)
{
    /* TODO: Implement
    auto it = this->usernameToID.find(username);

    if (it != std::end(this->usernameToID)) {
        return *it;
    }
    */

    static std::string temporary = "xd";
    return temporary;
}

void ChannelManager::doOnAll(std::function<void(std::shared_ptr<TwitchChannel>)> func)
{
    for (const auto &channel : this->twitchChannels) {
        func(std::get<0>(channel));
    }

    func(this->whispersChannel);
    func(this->mentionsChannel);
}

}  // namespace chatterino
