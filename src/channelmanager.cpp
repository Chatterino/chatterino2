#include "channelmanager.hpp"
#include "ircmanager.hpp"

namespace chatterino {

ChannelManager ChannelManager::instance;

ChannelManager::ChannelManager()
    : _channels()
    , _channelsMutex()
    , _whispers(new Channel(QString("/whispers")))
    , _mentions(new Channel(QString("/mentions")))
    , _empty(new Channel(QString("")))
{
}

SharedChannel ChannelManager::getWhispers()
{
    return _whispers;
}

SharedChannel ChannelManager::getMentions()
{
    return _mentions;
}

SharedChannel ChannelManager::getEmpty()
{
    return _empty;
}

const std::vector<SharedChannel> ChannelManager::getItems()
{
    QMutexLocker locker(&_channelsMutex);

    std::vector<SharedChannel> items;

    for (auto &item : _channels.values()) {
        items.push_back(std::get<0>(item));
    }

    return items;
}

SharedChannel ChannelManager::addChannel(const QString &channel)
{
    QMutexLocker locker(&_channelsMutex);

    QString channelName = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        return getChannel(channel);
    }

    auto it = _channels.find(channelName);

    if (it == _channels.end()) {
        auto channel = SharedChannel(new Channel(channelName));
        _channels.insert(channelName, std::make_tuple(channel, 1));

        IrcManager::getInstance().joinChannel(channelName);

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
        IrcManager::getInstance().partChannel(c);
        _channels.remove(c);
    }
}

}  // namespace chatterino
