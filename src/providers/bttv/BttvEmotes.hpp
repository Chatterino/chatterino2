#pragma once

#include <memory>

#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "messages/EmoteCache.hpp"

namespace chatterino {

class BttvEmotes final : std::enable_shared_from_this<BttvEmotes>
{
    static constexpr const char *globalEmoteApiUrl =
        "https://api.betterttv.net/2/emotes";

public:
    // BttvEmotes();

    AccessGuard<const EmoteMap> accessGlobalEmotes() const;
    boost::optional<EmotePtr> getGlobalEmote(const EmoteName &name);
    boost::optional<EmotePtr> getEmote(const EmoteId &id);

    void loadGlobalEmotes();

private:
    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonObject &jsonRoot, const EmoteMap &currentEmotes);

    UniqueAccess<EmoteMap> globalEmotes_;
    // UniqueAccess<WeakEmoteIdMap> channelEmoteCache_;
};

}  // namespace chatterino
