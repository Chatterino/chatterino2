#pragma once

#include "common/Aliases.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "providers/IvrApi.hpp"
#include "util/QStringHash.hpp"

#include <magic_enum.hpp>
#include <QColor>
#include <QRegularExpression>
#include <QString>

#include <memory>
#include <optional>
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

class ITwitchEmotes
{
public:
    virtual ~ITwitchEmotes() = default;

    virtual EmotePtr getOrCreateEmote(const EmoteId &id,
                                      const EmoteName &name) = 0;
};

struct TwitchEmote {
    QString id;
    QString name;

    // TODO: keep?
    QString emoteSetID;

    QString emoteType;
    QString ownerID;

    QStringList formats;
    QStringList scales;
    QStringList themeModes;
};

enum class TwitchEmoteType {
    // Subscription emotes are unlocked by subscribing to a channel
    Subscriptions,

    // Follower emotes are unlocked by following a channel
    Follower,

    // BitsTier emotes are unlocked by donating a set amount of bits to a channel
    BitsTier,

    // :-)
    Smilies,

    // Kappa
    Globals,

    // Turbo
    Turbo,

    // Prime
    Prime,

    LimitedTime,
};

struct TwitchEmoteSet {
    TwitchEmoteSet() = default;

    // TODO: enum class
    // This is inferred from the first emote in this set
    // Technically each emote in a set has its own emote type but right now they are always grouped up by type
    std::optional<TwitchEmoteType> emoteSetType;
    QString emoteSetTypeString;

    IvrEmoteSet ivrEmoteSet;

    EmoteMap emotes;

    // std::vector<TwitchEmote> emotes;
};

class TwitchEmotes : public ITwitchEmotes
{
public:
    static QString cleanUpEmoteCode(const QString &dirtyEmoteCode);
    TwitchEmotes() = default;

    EmotePtr getOrCreateEmote(const EmoteId &id,
                              const EmoteName &name) override;

    // TODO: Ensure emote sets can be reloaded if they're stale
    // How do we find out they're stale? Just 30 minutes old or something?
    void loadSets(QStringList emoteSets);

    std::unordered_map<QString, std::shared_ptr<TwitchEmoteSet>>
        twitchEmoteSets;

    // void registerTwitchEmoteSet(const QString &emoteSetID, std::weak_ptr<TwitchEmoteSet>)

private:
    Url getEmoteLink(const EmoteId &id, const QString &emoteScale);
    UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<Emote>>>
        twitchEmotesCache_;
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::TwitchEmoteType>(
        chatterino::TwitchEmoteType value) noexcept
{
    switch (value)
    {
        case chatterino::TwitchEmoteType::Subscriptions:
            return "SUBSCRIPTIONS";

        case chatterino::TwitchEmoteType::Follower:
            return "FOLLOWER";

        case chatterino::TwitchEmoteType::Smilies:
            return "SMILIES";

        case chatterino::TwitchEmoteType::Globals:
            return "GLOBALS";

        case chatterino::TwitchEmoteType::BitsTier:
            return "BITS_BADGE_TIERS";

        case chatterino::TwitchEmoteType::Turbo:
            return "TURBO";

        case chatterino::TwitchEmoteType::Prime:
            return "PRIME";

        case chatterino::TwitchEmoteType::LimitedTime:
            return "LIMITED_TIME";

        default:
            return default_tag;
    }
}
