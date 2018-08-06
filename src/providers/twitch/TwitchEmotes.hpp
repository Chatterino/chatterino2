#pragma once

#include <QString>
#include <unordered_map>

#include "common/Emotemap.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/ConcurrentMap.hpp"

#define TWITCH_EMOTE_TEMPLATE \
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}"

namespace chatterino {

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
