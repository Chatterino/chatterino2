#pragma once

#include <QString>

namespace chatterino {

class Env
{
    Env();

public:
    static const Env &get();

    const struct {
        QString url;
        uint32_t budget;
        uint32_t cooldown;
    } recentMessages;

    const QString linkResolverUrl;
    const QString twitchServerHost;
    const uint16_t twitchServerPort;
    const bool twitchServerSecure;
};

}  // namespace chatterino
