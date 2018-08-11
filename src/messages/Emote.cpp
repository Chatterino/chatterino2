#include "Emote.hpp"

#include <unordered_map>

namespace chatterino {

bool operator==(const Emote &a, const Emote &b)
{
    return std::tie(a.homePage, a.name, a.tooltip, a.images) ==
           std::tie(b.homePage, b.name, b.tooltip, b.images);
}

bool operator!=(const Emote &a, const Emote &b)
{
    return !(a == b);
}

EmotePtr cachedOrMakeEmotePtr(Emote &&emote, const EmoteMap &cache)
{
    // reuse old shared_ptr if nothing changed
    auto it = cache.find(emote.name);
    if (it != cache.end() && *it->second == emote) return it->second;

    return std::make_shared<Emote>(std::move(emote));
}

}  // namespace chatterino
