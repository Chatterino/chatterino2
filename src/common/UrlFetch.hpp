#pragma once

#include "common/NetworkRequest.hpp"

#include <QObject>
#include <QString>

namespace chatterino {

// Not sure if I like these, but I'm trying them out

static NetworkRequest makeGetChannelRequest(const QString &channelId,
                                            const QObject *caller = nullptr)
{
    QString url("https://api.twitch.tv/kraken/channels/" + channelId);

    auto request = NetworkRequest::twitchRequest(url);

    request.setCaller(caller);

    return request;
}

static NetworkRequest makeGetStreamRequest(const QString &channelId,
                                           const QObject *caller = nullptr)
{
    QString url("https://api.twitch.tv/kraken/streams/" + channelId);

    auto request = NetworkRequest::twitchRequest(url);

    request.setCaller(caller);

    return request;
}

}  // namespace chatterino
