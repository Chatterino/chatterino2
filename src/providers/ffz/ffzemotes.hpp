#pragma once

#include "signalvector.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <map>

namespace chatterino {
namespace providers {
namespace ffz {

class FFZEmotes
{
public:
    util::EmoteMap globalEmotes;
    SignalVector<std::string> globalEmoteCodes;

    util::EmoteMap channelEmotes;
    std::map<std::string, SignalVector<std::string>> channelEmoteCodes;

    void loadGlobalEmotes();

    void loadChannelEmotes(const QString &channelName,
                           std::weak_ptr<util::EmoteMap> channelEmoteMap);

    util::EmoteMap &getEmotes();
    util::ConcurrentMap<int, util::EmoteData> &getChannelEmoteFromCaches();

    util::ConcurrentMap<QString, util::EmoteMap> channels;

private:
    util::ConcurrentMap<int, util::EmoteData> _channelEmoteFromCaches;
};

}  // namespace ffz
}  // namespace providers
}  // namespace chatterino
