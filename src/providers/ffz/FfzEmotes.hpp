#pragma once

#include <memory>

#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "messages/EmoteCache.hpp"

namespace chatterino {

class FfzEmotes final : std::enable_shared_from_this<FfzEmotes>
{
    static constexpr const char *globalEmoteApiUrl = "https://api.frankerfacez.com/v1/set/global";
    static constexpr const char *channelEmoteApiUrl = "https://api.betterttv.net/2/channels/";

public:
    // FfzEmotes();

    static std::shared_ptr<FfzEmotes> create();

    AccessGuard<const EmoteCache<EmoteName>> accessGlobalEmotes() const;
    boost::optional<EmotePtr> getGlobalEmote(const EmoteName &name);
    boost::optional<EmotePtr> getEmote(const EmoteId &id);

    void loadGlobalEmotes();
    void loadChannelEmotes(const QString &channelName, std::function<void(EmoteMap &&)> callback);

protected:
    Outcome parseGlobalEmotes(const QJsonObject &jsonRoot);
    Outcome parseChannelEmotes(const QJsonObject &jsonRoot);

    UniqueAccess<EmoteCache<EmoteName>> globalEmotes_;
    UniqueAccess<WeakEmoteIdMap> channelEmoteCache_;
};

}  // namespace chatterino
