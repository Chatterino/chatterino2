#pragma once

#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/ConcurrentMap.hpp"
#include "common/Emotemap.hpp"

#include <map>

#include <QString>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchEmotes
{
public:
    TwitchEmotes();

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
        QString text;
        std::vector<TwitchEmote> emotes;
    };

    std::map<QString, EmoteSet> staticEmoteSets;

    struct TwitchAccountEmoteData {
        std::vector<std::shared_ptr<EmoteSet>> emoteSets;

        std::vector<QString> emoteCodes;

        util::EmoteMap emotes;

        bool filled = false;
    };

    // Key is the user ID
    std::map<QString, TwitchAccountEmoteData> emotes;

private:
    void loadSetData(std::shared_ptr<TwitchEmotes::EmoteSet> emoteSet);

    //            emote code
    util::ConcurrentMap<QString, providers::twitch::EmoteValue *> _twitchEmotes;

    //        emote id
    util::ConcurrentMap<QString, util::EmoteData> _twitchEmoteFromCache;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
