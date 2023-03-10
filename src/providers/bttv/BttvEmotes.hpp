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
struct BttvLiveUpdateEmoteUpdateAddMessage;
struct BttvLiveUpdateEmoteRemoveMessage;

class BttvEmotes final
{
    static constexpr const char *globalEmoteApiUrl =
        "https://api.betterttv.net/3/cached/emotes/global";
    static constexpr const char *bttvChannelEmoteApiUrl =
        "https://api.betterttv.net/3/cached/users/twitch/";

public:
    BttvEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static void loadChannel(std::weak_ptr<Channel> channel,
                            const QString &channelId,
                            const QString &channelDisplayName,
                            std::function<void(EmoteMap &&)> callback,
                            bool manualRefresh);

    /**
     * Adds an emote to the `channelEmoteMap`.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return The added emote.
     */
    static EmotePtr addEmote(
        const QString &channelDisplayName,
        Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
        const BttvLiveUpdateEmoteUpdateAddMessage &message);

    /**
     * Updates an emote in this `channelEmoteMap`.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return pair<old emote, new emote> if any emote was updated.
     */
    static boost::optional<std::pair<EmotePtr, EmotePtr>> updateEmote(
        const QString &channelDisplayName,
        Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
        const BttvLiveUpdateEmoteUpdateAddMessage &message);

    /**
     * Removes an emote from this `channelEmoteMap`.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return The removed emote if any emote was removed.
     */
    static boost::optional<EmotePtr> removeEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
        const BttvLiveUpdateEmoteRemoveMessage &message);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
