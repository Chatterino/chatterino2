#pragma once

#include "common/SignalVector.hpp"
#include "util/ConcurrentMap.hpp"
#include "common/Emotemap.hpp"

#include <map>

namespace chatterino {

class BTTVEmotes
{
public:
    EmoteMap globalEmotes;
    SignalVector<QString> globalEmoteCodes;

    EmoteMap channelEmotes;
    std::map<QString, SignalVector<QString>> channelEmoteCodes;

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName,
                           std::weak_ptr<EmoteMap> channelEmoteMap);

private:
    EmoteMap channelEmoteCache;
};

}  // namespace chatterino
