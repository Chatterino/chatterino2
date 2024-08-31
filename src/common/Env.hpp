#pragma once

#include <QString>

#include <optional>

namespace chatterino {

class Env
{
    Env();

public:
    static const Env &get();

    const QString recentMessagesApiUrl;
    const QString linkResolverUrl;
    const QString twitchServerHost;
    const uint16_t twitchServerPort;
    const bool twitchServerSecure;
    const std::optional<QString> proxyUrl;
};

}  // namespace chatterino
