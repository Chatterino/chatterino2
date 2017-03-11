#include "channels.h"
#include "ircmanager.h"

namespace chatterino {

static std::shared_ptr<Channel> whispers(nullptr);
static std::shared_ptr<Channel> mentions(nullptr);
static std::shared_ptr<Channel> empty(nullptr);

QMap<QString, std::tuple<std::shared_ptr<Channel>, int>> Channels::channels;
QMutex Channels::channelsMutex;

void
initChannels()
{
    whispers.reset(new Channel(QString("/whispers")));
    mentions.reset(new Channel(QString("/mentions")));
    empty.reset(new Channel(QString("")));
}

std::shared_ptr<Channel>
Channels::getWhispers()
{
    return whispers;
}

std::shared_ptr<Channel>
Channels::getMentions()
{
    return mentions;
}

std::shared_ptr<Channel>
Channels::getEmpty()
{
    return empty;
}

const std::vector<std::shared_ptr<Channel>>
Channels::getItems()
{
    QMutexLocker locker(&Channels::channelsMutex);

    std::vector<std::shared_ptr<Channel>> items;

    for (auto &item : Channels::channels.values()) {
        items.push_back(std::get<0>(item));
    }

    return items;
}

std::shared_ptr<Channel>
Channels::addChannel(const QString &channel)
{
    QMutexLocker locker(&Channels::channelsMutex);

    QString channelName = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        return getChannel(channel);
    }

    auto it = Channels::channels.find(channelName);

    if (it == Channels::channels.end()) {
        auto channel = std::shared_ptr<Channel>(new Channel(channelName));
        Channels::channels.insert(channelName, std::make_tuple(channel, 1));

        IrcManager::sendJoin(channelName);

        return channel;
    }

    std::get<1>(it.value())++;

    return std::get<0>(it.value());
}

std::shared_ptr<Channel>
Channels::getChannel(const QString &channel)
{
    QMutexLocker locker(&Channels::channelsMutex);

    QString c = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        if (c == "/whispers") {
            return whispers;
        }

        if (c == "/mentions") {
            return mentions;
        }

        return empty;
    }

    auto a = Channels::channels.find(c);

    if (a == Channels::channels.end()) {
        return empty;
    }

    return std::get<0>(a.value());
}

void
Channels::removeChannel(const QString &channel)
{
    QMutexLocker locker(&Channels::channelsMutex);

    if (channel.length() > 1 && channel.at(0) == '/') {
        return;
    }

    QString c = channel.toLower();

    auto a = Channels::channels.find(c);

    if (a == Channels::channels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        IrcManager::partChannel(c);
        Channels::channels.remove(c);
    }
}

}  // namespace chatterino
