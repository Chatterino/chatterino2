#pragma once

#include <QString>

namespace chatterino {

class Env
{
    Env();

public:
    static const Env &get();

    const QString recentMessagesApiUrl;
    const QString linkResolverUrl;
    const QString twitchEmoteSetResolverUrl;
    const QString imageUploaderUrl;
    const QString imageUploaderFormBody;
    const QString twitchServerHost;
    const uint16_t twitchServerPort;
    const bool twitchServerSecure;
};

}  // namespace chatterino
