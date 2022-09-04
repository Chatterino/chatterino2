#pragma once

#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace chatterino {

struct Emote {
    EmoteName name;
    ImageSet images;
    Tooltip tooltip;
    Url homePage;
    bool zeroWidth;

    // FOURTF: no solution yet, to be refactored later
    const QString &getCopyString() const
    {
        return name.string;
    }
};

bool operator==(const Emote &a, const Emote &b);
bool operator!=(const Emote &a, const Emote &b);

using EmotePtr = std::shared_ptr<const Emote>;

class EmoteMap : public std::unordered_map<EmoteName, EmotePtr>
{
};
using EmoteIdMap = std::unordered_map<EmoteId, EmotePtr>;
using WeakEmoteMap = std::unordered_map<EmoteName, std::weak_ptr<const Emote>>;
using WeakEmoteIdMap = std::unordered_map<EmoteId, std::weak_ptr<const Emote>>;

static const std::shared_ptr<const EmoteMap> EMPTY_EMOTE_MAP = std::make_shared<
    const EmoteMap>();  // NOLINT(cert-err58-cpp) -- assume this doesn't throw an exception

EmotePtr cachedOrMakeEmotePtr(Emote &&emote, const EmoteMap &cache);
EmotePtr cachedOrMakeEmotePtr(
    Emote &&emote,
    std::unordered_map<EmoteId, std::weak_ptr<const Emote>> &cache,
    std::mutex &mutex, const EmoteId &id);

}  // namespace chatterino
