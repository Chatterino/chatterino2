#include "channelmanager.hpp"
#include "ircmanager.hpp"

namespace chatterino {

ChannelManager::ChannelManager(WindowManager &_windowManager, EmoteManager &_emoteManager,
                               IrcManager &_ircManager)
    : windowManager(_windowManager)
    , emoteManager(_emoteManager)
    , ircManager(_ircManager)
    , whispersChannel(new Channel(_windowManager, _emoteManager, _ircManager, "/whispers", true))
    , mentionsChannel(new Channel(_windowManager, _emoteManager, _ircManager, "/mentions", true))
    , emptyChannel(new Channel(_windowManager, _emoteManager, _ircManager, "", true))
{
}

const std::vector<std::shared_ptr<Channel>> ChannelManager::getItems()
{
    QMutexLocker locker(&this->channelsMutex);

    std::vector<std::shared_ptr<Channel>> items;

    for (auto &item : this->channels.values()) {
        items.push_back(std::get<0>(item));
    }

    return items;
}

std::shared_ptr<Channel> ChannelManager::addChannel(const QString &rawChannelName)
{
    QString channelName = rawChannelName.toLower();

    if (channelName.length() > 1 && channelName.at(0) == '/') {
        return this->getChannel(channelName);
    }

    if (channelName.length() > 0 && channelName.at(0) == '#') {
        channelName = channelName.mid(1);
    }

    QMutexLocker locker(&this->channelsMutex);

    auto it = this->channels.find(channelName);

    if (it == this->channels.end()) {
        auto channel = std::make_shared<Channel>(this->windowManager, this->emoteManager,
                                                 this->ircManager, channelName);
        this->channels.insert(channelName, std::make_tuple(channel, 1));

        this->ircManager.joinChannel(channelName);

        return channel;
    }

    std::get<1>(it.value())++;

    return std::get<0>(it.value());
}

std::shared_ptr<Channel> ChannelManager::getChannel(const QString &channel)
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

    auto a = this->channels.find(c);

    if (a == this->channels.end()) {
        return emptyChannel;
    }

    return std::get<0>(a.value());
}

void ChannelManager::removeChannel(const QString &channel)
{
    QMutexLocker locker(&this->channelsMutex);

    if (channel.length() > 1 && channel.at(0) == '/') {
        return;
    }

    QString c = channel.toLower();

    auto a = this->channels.find(c);

    if (a == this->channels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        this->ircManager.partChannel(c);
        this->channels.remove(c);
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
