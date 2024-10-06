#pragma once

#include "common/Aliases.hpp"
#include "messages/ImageSet.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

class QJsonObject;

namespace chatterino {

struct Emote {
    EmoteName name;
    ImageSet images;
    Tooltip tooltip;
    Url homePage;
    bool zeroWidth{};
    EmoteId id;
    EmoteAuthor author;
    /**
     * If this emote is aliased, this contains
     * the original (base) name of the emote.
     */
    std::optional<EmoteName> baseName;

    // FOURTF: no solution yet, to be refactored later
    const QString &getCopyString() const
    {
        return name.string;
    }

    QJsonObject toJson() const;
};

bool operator==(const Emote &a, const Emote &b);
bool operator!=(const Emote &a, const Emote &b);

using EmotePtr = std::shared_ptr<const Emote>;

class EmoteMap : public std::unordered_map<EmoteName, EmotePtr>
{
public:
    /**
     * Finds an emote by it's id with a hint to it's name.
     *
     * 1. Searches by name for the emote, checking if the ids match (fast-path).
     * 2. Searches through the map for an emote with the `emoteID` (slow-path).
     *
     * @param emoteNameHint A hint to the name of the searched emote,
     *                      may be empty.
     * @param emoteID The emote id to search for.
     * @return An iterator to the found emote (possibly this->end()).
     */
    EmoteMap::const_iterator findEmote(const QString &emoteNameHint,
                                       const QString &emoteID) const;
};

inline const std::shared_ptr<const EmoteMap> EMPTY_EMOTE_MAP = std::make_shared<
    const EmoteMap>();  // NOLINT(cert-err58-cpp) -- assume this doesn't throw an exception

EmotePtr cachedOrMakeEmotePtr(Emote &&emote, const EmoteMap &cache);
EmotePtr cachedOrMakeEmotePtr(
    Emote &&emote,
    std::unordered_map<EmoteId, std::weak_ptr<const Emote>> &cache,
    std::mutex &mutex, const EmoteId &id);

}  // namespace chatterino
