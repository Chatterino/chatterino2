#pragma once

#include "common/Aliases.hpp"
#include "messages/Emote.hpp"
#include "messages/ImageSet.hpp"

namespace chatterino {
class SeventvEmoteCache
{
public:
    /**
     * Adds the emote to the cache.
     * If the emote's images were cached, they're erased from the cache.
     */
    EmotePtr putEmote(const EmoteId &id, Emote &&emote);
    /**
     * Retrieves the cached emote.
     * If it doesn't exist, a default constructed shared_ptr is returned (nullptr)
     */
    EmotePtr getEmote(const EmoteId &id);
    /**
     * Adds the image set to the cache.
     */
    void putImageSet(const EmoteId &id, const ImageSet &set);
    /**
     * Retrieves the cached image-set. If it doesn't exist, returns a nullptr.
     */
    WeakImageSet *getImageSet(const EmoteId &id);

private:
    /**
     * This cache only contains unaliased, non-global emotes.
     */
    std::unordered_map<EmoteId, std::weak_ptr<const Emote>> emoteCache_ = {};
    /**
     * This cache only contains image-sets where the emote isn't yet cached.
     */
    std::unordered_map<EmoteId, WeakImageSet> imageCache_ = {};
};
}  // namespace chatterino