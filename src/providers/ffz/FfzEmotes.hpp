#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"

#include <boost/optional.hpp>

#include <memory>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;
class Channel;

class FfzEmotes final
{
public:
    FfzEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static void loadChannel(
        std::weak_ptr<Channel> channel, const QString &channelId,
        std::function<void(EmoteMap &&)> emoteCallback,
        std::function<void(boost::optional<EmotePtr>)> modBadgeCallback,
        std::function<void(boost::optional<EmotePtr>)> vipBadgeCallback,
        bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
