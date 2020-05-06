#pragma once

#include <memory>
#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class FfzEmotes final
{
    static constexpr const char *globalEmoteApiUrl =
        "https://api.frankerfacez.com/v1/set/global";

public:
    FfzEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static void loadChannel(
        TwitchChannel &channel, const QString &channelId,
        std::function<void(EmoteMap &&)> emoteCallback,
        std::function<void(boost::optional<EmotePtr>)> modBadgeCallback);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
