#pragma once

#include "SignalVector.hpp"
#include "util/ConcurrentMap.hpp"
#include "util/Emotemap.hpp"

#include <map>

namespace chatterino {
namespace providers {
namespace bttv {

class BTTVEmotes
{
public:
    util::EmoteMap globalEmotes;
    SignalVector<QString> globalEmoteCodes;

    util::EmoteMap channelEmotes;
    std::map<QString, SignalVector<QString>> channelEmoteCodes;

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName,
                           std::weak_ptr<util::EmoteMap> channelEmoteMap);

private:
    util::EmoteMap channelEmoteCache;
};

}  // namespace bttv
}  // namespace providers
}  // namespace chatterino
