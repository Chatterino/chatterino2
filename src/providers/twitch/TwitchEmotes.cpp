#include "providers/twitch/TwitchEmotes.hpp"

#include "common/NetworkRequest.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

TwitchEmotes::TwitchEmotes()
{
}

QString TwitchEmotes::cleanUpEmoteCode(const QString &dirtyEmoteCode)
{
    auto cleanCode = dirtyEmoteCode;
    cleanCode.detach();

    static QMap<QString, QString> emoteNameReplacements{
        {"[oO](_|\\.)[oO]", "O_o"}, {"\\&gt\\;\\(", "&gt;("},
        {"\\&lt\\;3", "&lt;3"},     {"\\:-?(o|O)", ":O"},
        {"\\:-?(p|P)", ":P"},       {"\\:-?[\\\\/]", ":/"},
        {"\\:-?[z|Z|\\|]", ":Z"},   {"\\:-?\\(", ":("},
        {"\\:-?\\)", ":)"},         {"\\:-?D", ":D"},
        {"\\;-?(p|P)", ";P"},       {"\\;-?\\)", ";)"},
        {"R-?\\)", "R)"},           {"B-?\\)", "B)"},
    };

    auto it = emoteNameReplacements.find(dirtyEmoteCode);
    if (it != emoteNameReplacements.end())
    {
        cleanCode = it.value();
    }

    return cleanCode;
}

// id is used for lookup
// emoteName is used for giving a name to the emote in case it doesn't exist
EmotePtr TwitchEmotes::getOrCreateEmote(const EmoteId &id,
                                        const EmoteName &name_)
{
    auto name = TwitchEmotes::cleanUpEmoteCode(name_.string);

    // search in cache or create new emote
    auto cache = this->twitchEmotesCache_.access();
    auto shared = (*cache)[id].lock();

    if (!shared)
    {
        (*cache)[id] = shared = std::make_shared<Emote>(Emote{
            EmoteName{name},
            ImageSet{
                Image::fromUrl(getEmoteLink(id, "1.0"), 1),
                Image::fromUrl(getEmoteLink(id, "2.0"), 0.5),
                Image::fromUrl(getEmoteLink(id, "3.0"), 0.25),
            },
            Tooltip{name.toHtmlEscaped() + "<br>Twitch Emote"},
        });
    }

    return shared;
}

Url TwitchEmotes::getEmoteLink(const EmoteId &id, const QString &emoteScale)
{
    return {QString(TWITCH_EMOTE_TEMPLATE)
                .replace("{id}", id.string)
                .replace("{scale}", emoteScale)};
}

}  // namespace chatterino
