#pragma once

#include <memory>
#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"

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
    static void loadChannel(const QString &channelName,
                            std::function<void(EmoteMap &&)> callback);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
