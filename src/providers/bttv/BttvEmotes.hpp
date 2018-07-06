#pragma once

#include "common/Emotemap.hpp"
#include "common/SimpleSignalVector.hpp"
#include "util/ConcurrentMap.hpp"

#include <map>

namespace chatterino {

class BTTVEmotes
{
public:
    EmoteMap globalEmotes;
    SimpleSignalVector<QString> globalEmoteCodes;

    EmoteMap channelEmotes;
    std::map<QString, SimpleSignalVector<QString>> channelEmoteCodes;

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName, std::weak_ptr<EmoteMap> channelEmoteMap);

private:
    EmoteMap channelEmoteCache_;
};

}  // namespace chatterino
