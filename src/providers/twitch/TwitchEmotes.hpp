#pragma once

#include "common/Emotemap.hpp"
#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/ConcurrentMap.hpp"

#include <map>

#include <QString>

namespace chatterino {

class TwitchEmotes
{
public:
    TwitchEmotes();

    EmoteData getEmoteById(const QString &id, const QString &emoteName);

    /// Twitch emotes
    void refresh(const std::shared_ptr<TwitchAccount> &user);

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

        EmoteMap emotes;

        bool filled = false;
    };

    // Key is the user ID
    std::map<QString, TwitchAccountEmoteData> emotes;

private:
    void loadSetData(std::shared_ptr<TwitchEmotes::EmoteSet> emoteSet);

    //            emote code
    ConcurrentMap<QString, EmoteValue *> twitchEmotes_;

    //            emote id
    ConcurrentMap<QString, EmoteData> twitchEmoteFromCache_;
};

}  // namespace chatterino
