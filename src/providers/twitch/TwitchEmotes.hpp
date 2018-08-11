#pragma once

#include <QString>
#include <unordered_map>

#include "common/Aliases.hpp"
#include "common/UniqueAccess.hpp"
#include "providers/twitch/TwitchEmotes.hpp"

#define TWITCH_EMOTE_TEMPLATE \
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class TwitchEmotes
{
public:
    TwitchEmotes();

    EmotePtr getOrCreateEmote(const EmoteId &id, const EmoteName &name);
    Url getEmoteLink(const EmoteId &id, const QString &emoteScale);
    AccessGuard<std::unordered_map<EmoteName, EmotePtr>> accessAll();

private:
    UniqueAccess<std::unordered_map<EmoteName, EmotePtr>> twitchEmotes_;
    UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<Emote>>>
        twitchEmotesCache_;
};

}  // namespace chatterino
