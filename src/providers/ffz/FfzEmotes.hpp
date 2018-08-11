#pragma once

#include <memory>
#include "common/Atomic.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

class FfzEmotes final
{
    static constexpr const char *globalEmoteApiUrl =
        "https://api.frankerfacez.com/v1/set/global";
    static constexpr const char *channelEmoteApiUrl =
        "https://api.betterttv.net/2/channels/";

public:
    FfzEmotes();

    std::shared_ptr<const EmoteMap> global() const;
    boost::optional<EmotePtr> global(const EmoteName &name) const;
    void loadGlobal();
    static void loadChannel(const QString &channelName,
                            std::function<void(EmoteMap &&)> callback);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
