#pragma once

#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <memory>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class SeventvEmotes final
{
    static constexpr const char *apiUrlGQL = "https://api.7tv.app/v2/gql";

public:
    SeventvEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static void loadChannel(std::weak_ptr<Channel> channel,
                            const QString &channelId,
                            std::function<void(EmoteMap &&)> callback,
                            bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
