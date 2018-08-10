#pragma once

#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

QStringAlias(EmoteId);
QStringAlias(EmoteName);

namespace chatterino {

struct Emote {
    EmoteName name;
    ImageSet images;
    Tooltip tooltip;
    Url homePage;

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

}  // namespace chatterino
