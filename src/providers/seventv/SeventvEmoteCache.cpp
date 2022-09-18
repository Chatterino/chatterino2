#include "SeventvEmoteCache.hpp"

namespace chatterino {
EmotePtr SeventvEmoteCache::putEmote(const EmoteId &id, Emote &&emote)
{
    if (auto it = this->imageCache_.find(id); it != this->imageCache_.end())
    {
        this->imageCache_.erase(it);
    }
    auto ptr = std::make_shared<Emote>(std::move(emote));
    this->emoteCache_[id] = ptr;
    return ptr;
}

EmotePtr SeventvEmoteCache::getEmote(const EmoteId &id)
{
    return this->emoteCache_[id].lock();
}

void SeventvEmoteCache::putImageSet(const EmoteId &id, const ImageSet &set)
{
    this->imageCache_.emplace(id, set);
}

WeakImageSet *SeventvEmoteCache::getImageSet(const EmoteId &id)
{
    if (auto it = this->imageCache_.find(id); it != this->imageCache_.end())
    {
        return &it->second;
    }
    return nullptr;
}
}  // namespace chatterino