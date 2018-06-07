#pragma once

#include "signalvector.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <map>

namespace chatterino {
namespace providers {
namespace bttv {

class BTTVEmotes
{
public:
    util::EmoteMap globalEmotes;
    SignalVector<std::string> globalEmoteCodes;

    util::EmoteMap channelEmotes;
    std::map<std::string, SignalVector<std::string>> channelEmoteCodes;

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName,
                           std::weak_ptr<util::EmoteMap> channelEmoteMap);

private:
    util::EmoteMap channelEmoteCache;
};

}  // namespace bttv
}  // namespace providers
}  // namespace chatterino
