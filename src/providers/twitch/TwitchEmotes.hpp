#pragma once

#include "common/Aliases.hpp"
#include "common/UniqueAccess.hpp"
#include "providers/twitch/TwitchUser.hpp"

#include <boost/unordered/unordered_flat_map_fwd.hpp>
#include <QColor>
#include <QRegularExpression>
#include <QString>

#include <memory>
#include <unordered_map>

// NB: "default" can be replaced with "static" to always get a non-animated
// variant
#define TWITCH_EMOTE_TEMPLATE \
    "https://static-cdn.jtvnw.net/emoticons/v2/{id}/default/dark/{scale}"

namespace chatterino {
struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

struct CheerEmote {
    QColor color;
    int minBits;
    QRegularExpression regex;

    EmotePtr animatedEmote;
    EmotePtr staticEmote;
};

struct CheerEmoteSet {
    QRegularExpression regex;
    std::vector<CheerEmote> cheerEmotes;
};

struct TwitchEmoteSet {
    /// @brief The owner of this set
    ///
    /// This owner might not be resolved yet
    std::shared_ptr<TwitchUser> owner;

    std::vector<EmotePtr> emotes;

    /// @brief If this is subscriber-like emote set
    ///
    /// This is true for follower, subscriber and bitstier emote sets.
    bool isSubLike = false;
    bool isFollower = false;

    QString title() const;
};
using TwitchEmoteSetMap = boost::unordered_flat_map<EmoteSetId, TwitchEmoteSet>;

class ITwitchEmotes
{
public:
    virtual ~ITwitchEmotes() = default;

    virtual EmotePtr getOrCreateEmote(const EmoteId &id,
                                      const EmoteName &name) = 0;
};

class TwitchEmotes : public ITwitchEmotes
{
public:
    static QString cleanUpEmoteCode(const QString &dirtyEmoteCode);
    TwitchEmotes() = default;

    EmotePtr getOrCreateEmote(const EmoteId &id,
                              const EmoteName &name) override;

private:
    Url getEmoteLink(const EmoteId &id, const QString &emoteScale);
    UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<Emote>>>
        twitchEmotesCache_;
};

}  // namespace chatterino
