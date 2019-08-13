#pragma once

#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <unordered_map>

#include "common/Aliases.hpp"
#include "common/UniqueAccess.hpp"

#define TWITCH_EMOTE_TEMPLATE \
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

namespace chatterino {
struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

struct CheerEmote {
    QColor color;
    int minBits;

    EmotePtr animatedEmote;
    EmotePtr staticEmote;
};

struct CheerEmoteSet {
    QRegularExpression regex;
    std::vector<CheerEmote> cheerEmotes;
};

class TwitchEmotes
{
public:
    static QString cleanUpEmoteCode(const EmoteName &dirtyEmoteCode);
    TwitchEmotes();

    EmotePtr getOrCreateEmote(const EmoteId &id, const EmoteName &name);

private:
    Url getEmoteLink(const EmoteId &id, const QString &emoteScale);
    UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<Emote>>>
        twitchEmotesCache_;

    std::mutex mutex_;
};

}  // namespace chatterino
