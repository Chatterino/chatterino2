#include "channels.h"
#include "ircmanager.h"

Channel Channels::whispers(QString("/whispers"));
Channel Channels::mentions(QString("/mentions"));
Channel Channels::empty(QString(""));

QMap<QString, std::tuple<Channel *, int>> Channels::channels;

Channel *
Channels::addChannel(const QString &channel)
{
    QString c = channel.toLower();

    auto a = Channels::channels.find(c);

    if (a == Channels::channels.end()) {
        auto _c = new Channel(c);
        Channels::channels.insert(c, std::tuple<Channel *, int>(_c, 1));

        IrcManager::joinChannel(c);

        return _c;
    } else {
        std::get<1>(a.value())++;
    }

    return std::get<0>(a.value());
}

Channel *
Channels::getChannel(const QString &channel)
{
    QString c = channel.toLower();

    if (channel.length() > 1 && channel.at(0) == '/') {
        if (c == "/whispers") {
            return &Channels::whispers;
        }

        if (c == "/mentions") {
            return &Channels::mentions;
        }

        return &Channels::empty;
    }

    auto a = Channels::channels.find(c);

    if (a == Channels::channels.end()) {
        return NULL;
    }

    return std::get<0>(a.value());
}

void
Channels::removeChannel(const QString &channel)
{
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
        delete std::get<0>(a.value());
    }
}
