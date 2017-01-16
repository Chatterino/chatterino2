#include "channels.h"

Channel Channels::m_whispers(QString("/whispers"));
Channel Channels::m_mentions(QString("/mentions"));

QMap<QString, std::tuple<Channel *, int>> Channels::m_channels;

Channel *
Channels::addChannel(const QString &channel)
{
    auto c = getChannel(channel);

    if (c == NULL) {
        c = new Channel(channel);
        m_channels.insert(channel, std::tuple<Channel *, int>(c, 1));

        return c;
    }

    return c;
}

Channel *
Channels::getChannel(const QString &channel)
{
    if (channel == "/whispers") {
        return &m_whispers;
    }

    if (channel == "/mentions") {
        return &m_mentions;
    }

    auto a = m_channels.find(channel);

    if (a == m_channels.end()) {
        return NULL;
    }

    return std::get<0>(a.value());
}

void
Channels::removeChannel(const QString &channel)
{
    auto a = m_channels.find(channel);

    if (a == m_channels.end()) {
        return;
    }

    std::get<1>(a.value())--;

    if (std::get<1>(a.value()) == 0) {
        m_channels.remove(channel);
        delete std::get<0>(a.value());
    }
}
