#pragma once

#include "common/Emotemap.hpp"
#include "common/SignalVector.hpp"
#include "util/ConcurrentMap.hpp"

#include <map>

namespace chatterino {

class FFZEmotes
{
public:
    EmoteMap globalEmotes;
    SignalVector<QString> globalEmoteCodes;

    EmoteMap channelEmotes;
    std::map<QString, SignalVector<QString>> channelEmoteCodes;

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName, std::weak_ptr<EmoteMap> channelEmoteMap);

private:
    ConcurrentMap<int, EmoteData> channelEmoteCache;
};

}  // namespace chatterino
