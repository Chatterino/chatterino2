#include "channelmanager.hpp"
#include "ircmanager.hpp"

namespace chatterino {

ChannelManager::ChannelManager(WindowManager &_windowManager, EmoteManager &_emoteManager,
                               IrcManager &_ircManager)
    : windowManager(_windowManager)
    , emoteManager(_emoteManager)
    , ircManager(_ircManager)
    , _whispers(new Channel(_windowManager, _emoteManager, _ircManager, "/whispers", true))
    , _mentions(new Channel(_windowManager, _emoteManager, _ircManager, "/mentions", true))
    , _empty(new Channel(_windowManager, _emoteManager, _ircManager, QString(), true))
{
}

std::shared_ptr<Channel> ChannelManager::getWhispers()
{
    return _whispers;
}

std::shared_ptr<Channel> ChannelManager::getMentions()
{
    return _mentions;
}

std::shared_ptr<Channel> ChannelManager::getEmpty()
{
    return _empty;
}

const std::vector<std::shared_ptr<Channel>> ChannelManager::getItems()
{
    QMutexLocker locker(&_channelsMutex);

    std::vector<std::shared_ptr<Channel>> items;

    for (auto &item : _channels.values()) {
        items.push_back(std::get<0>(item));
    }

    return items;
}

std::shared_ptr<Channel> ChannelManager::addChannel(const QString &channel)
{
    QMutexLocker locker(&_channelsMutex);

    QString channelName = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        return getChannel(channel);
    }

    auto it = _channels.find(channelName);

    if (it == _channels.end()) {
        auto channel = std::shared_ptr<Channel>(
            new Channel(this->windowManager, this->emoteManager, this->ircManager, channelName));
        _channels.insert(channelName, std::make_tuple(channel, 1));

        this->ircManager.joinChannel(channelName);

        return channel;
    }

    std::get<1>(it.value())++;

    return std::get<0>(it.value());
}

std::shared_ptr<Channel> ChannelManager::getChannel(const QString &channel)
{
    QMutexLocker locker(&_channelsMutex);

    QString c = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        if (c == "/whispers") {
            return _whispers;
        }

        if (c == "/mentions") {
            return _mentions;
        }

        return _empty;
    }

    auto a = _channels.find(c);

    if (a == _channels.end()) {
        return _empty;
    }

    return std::get<0>(a.value());
}

void ChannelManager::removeChannel(const QString &channel)
{
    QMutexLocker locker(&_channelsMutex);

    if (channel.length() > 1 && channel.at(0) == '/') {
        return;
    }

    QString c = channel.toLower();

    auto a = _channels.find(c);

    if (a == _channels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        this->ircManager.partChannel(c);
        _channels.remove(c);
    }
}

const std::string &ChannelManager::getUserID(const std::string &username)
{
    auto it = this->usernameToID.find(username);

    /*
    if (it != std::end(this->usernameToID)) {
        return *it;
    }
    */

    return "xd";
}

}  // namespace chatterino
