#pragma once

#include <memory>
#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class BttvEmotes final
{
    static constexpr const char *globalEmoteApiUrl =
        "https://api.betterttv.net/2/emotes";
    static constexpr const char *bttvChannelEmoteApiUrl =
        "https://api.betterttv.net/2/channels/";

public:
    BttvEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static void loadChannel(const QString &channelName,
                            std::function<void(EmoteMap &&)> callback);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
