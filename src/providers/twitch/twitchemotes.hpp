#pragma once

#include "providers/twitch/emotevalue.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchemotes.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <map>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchEmotes
{
public:
    util::EmoteData getEmoteById(long int id, const QString &emoteName);

    /// Twitch emotes
    void refresh(const std::shared_ptr<providers::twitch::TwitchAccount> &user);

    struct TwitchAccountEmoteData {
        struct TwitchEmote {
            std::string id;
            std::string code;
        };

        //       emote set
        std::map<std::string, std::vector<TwitchEmote>> emoteSets;

        std::vector<std::string> emoteCodes;

        util::EmoteMap emotes;

        bool filled = false;
    };

    std::map<std::string, TwitchAccountEmoteData> emotes;

private:
    //            emote code
    util::ConcurrentMap<QString, providers::twitch::EmoteValue *> _twitchEmotes;

    //        emote id
    util::ConcurrentMap<long, util::EmoteData> _twitchEmoteFromCache;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
