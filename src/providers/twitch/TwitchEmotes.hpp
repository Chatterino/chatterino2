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

namespace chatterino {

// NB: "default" can be replaced with "static" to always get a non-animated
// variant
/// %1 <-> {id}
/// %2 <-> {scale} (1.0, 2.0, 3.0)
inline constexpr QStringView TWITCH_EMOTE_TEMPLATE =
    u"https://static-cdn.jtvnw.net/emoticons/v2/%1/default/dark/%2";

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

    /// If this is a bitstier emote set
    bool isBits = false;

    /// @brief If this emote set is a subscriber or similar emote set
    ///
    /// This includes sub and bit emotes
    bool isSubLike = false;

    /// @brief The title of this set
    ///
    /// We generate this based on the emote set's flags & owner
    QString title() const;
};
using TwitchEmoteSetMap = boost::unordered_flat_map<EmoteSetId, TwitchEmoteSet>;

struct HelixChannelEmote;

inline constexpr QStringView TWITCH_SUB_EMOTE_SET_PREFIX = u"x-c2-s-";
inline constexpr QStringView TWITCH_BIT_EMOTE_SET_PREFIX = u"x-c2-b-";

struct TwitchEmoteSetMeta {
    QString setID;

    /// See TwitchEmoteSet::isBits
    bool isBits = false;
    /// See TwitchEmoteSet::isSubLike
    bool isSubLike = false;
};

TwitchEmoteSetMeta getTwitchEmoteSetMeta(const HelixChannelEmote &emote);

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
    UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<Emote>>>
        twitchEmotesCache_;
};

}  // namespace chatterino
