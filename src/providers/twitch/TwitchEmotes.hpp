#pragma once

#include "common/Aliases.hpp"
#include "common/UniqueAccess.hpp"

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
