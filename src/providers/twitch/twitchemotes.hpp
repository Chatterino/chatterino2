#pragma once

#include "providers/twitch/emotevalue.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchemotes.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <map>

#include <QString>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchEmotes
{
public:
    util::EmoteData getEmoteById(const QString &id, const QString &emoteName);

    /// Twitch emotes
    void refresh(const std::shared_ptr<providers::twitch::TwitchAccount> &user);

    struct TwitchEmote {
        TwitchEmote(const QString &_id, const QString &_code)
            : id(_id)
            , code(_code)
        {
        }

        // i.e. "403921"
        QString id;

        // i.e. "forsenE"
        QString code;
    };

    struct EmoteSet {
        QString key;
        QString channelName;
        std::vector<TwitchEmote> emotes;
    };

    // std::map<QString, SubscriptionChannel> subscriberSet;
    std::map<std::string, EmoteSet> emoteSets;

    struct TwitchAccountEmoteData {
        std::vector<std::shared_ptr<EmoteSet>> emoteSets;

        std::vector<QString> emoteCodes;

        util::EmoteMap emotes;

        bool filled = false;
    };

    // Key is the user ID
    std::map<QString, TwitchAccountEmoteData> emotes;

private:
    //            emote code
    util::ConcurrentMap<QString, providers::twitch::EmoteValue *> _twitchEmotes;

    //        emote id
    util::ConcurrentMap<QString, util::EmoteData> _twitchEmoteFromCache;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
