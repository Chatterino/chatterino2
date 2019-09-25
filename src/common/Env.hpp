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
};

}  // namespace chatterino
