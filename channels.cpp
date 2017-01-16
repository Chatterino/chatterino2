#include "channels.h"
#include "ircmanager.h"

Channel Channels::m_whispers(QString("/whispers"));
Channel Channels::m_mentions(QString("/mentions"));
Channel Channels::m_empty(QString(""));

QMap<QString, std::tuple<Channel *, int>> Channels::m_channels;

Channel *
Channels::addChannel(const QString &channel)
{
    QString c = channel.toLower();

    auto a = m_channels.find(c);

    if (a == m_channels.end()) {
        auto _c = new Channel(c);
        m_channels.insert(c, std::tuple<Channel *, int>(_c, 1));

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
            return &m_whispers;
        }

        if (c == "/mentions") {
            return &m_mentions;
        }

        return &m_empty;
    }

    auto a = m_channels.find(c);

    if (a == m_channels.end()) {
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

    auto a = m_channels.find(c);

    if (a == m_channels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        IrcManager::partChannel(c);
        m_channels.remove(c);
        delete std::get<0>(a.value());
    }
}
