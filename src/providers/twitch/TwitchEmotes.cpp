#include "providers/twitch/TwitchEmotes.hpp"

#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/IvrApi.hpp"
#include "util/QStringHash.hpp"

#include <cmath>
#include <optional>

namespace chatterino {

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
                                        const EmoteName &name_,
                                        TwitchEmoteData &&extraEmoteData)
{
    auto name = TwitchEmotes::cleanUpEmoteCode(name_.string);

    // search in cache or create new emote
    auto cache = this->twitchEmotesCache_.access();
    auto shared = (*cache)[id].lock();

    if (!shared)
    {
        auto emote = std::make_shared<Emote>(Emote{
            EmoteName{name},
            ImageSet{
                Image::fromUrl(getEmoteLink(id, "1.0"), 1),
                Image::fromUrl(getEmoteLink(id, "2.0"), 0.5),
                Image::fromUrl(getEmoteLink(id, "3.0"), 0.25),
            },
            Tooltip{name.toHtmlEscaped() + "<br>Twitch Emote"},
        });

        if (extraEmoteData.type)
        {
            emote->type = *extraEmoteData.type;
            if (emote->type == Emote::Type::TwitchGlobal)
            {
                emote->tooltip =
                    Tooltip{name.toHtmlEscaped() + "<br>Twitch Global Emote"};
            }
        }

        if (extraEmoteData.author)
        {
            emote->author = {*extraEmoteData.author};

            emote->tooltip = Tooltip{emote->tooltip.string + "<br> from " +
                                     emote->author.string};
        }

        (*cache)[id] = shared = emote;
    }

    return shared;
}

template <typename C>
auto splitListIntoBatches2(const C &container, int batchSize = 100)
{
    std::vector<C> batches;
    int batchCount =
        std::ceil(static_cast<double>(container.size()) / batchSize);
    batches.reserve(batchCount);

    auto it = container.cbegin();

    for (int j = 0; j < batchCount; j++)
    {
        C batch;

        for (int i = 0; i < batchSize && it != container.end(); i++)
        {
            batch.insert(it.key(), it.value());
            it++;
        }
        if (batch.empty())
        {
            break;
        }
        batches.emplace_back(std::move(batch));
    }

    return batches;
}

void TwitchEmotes::loadSets(QStringList emoteSets)
{
    QMap<QString, std::shared_ptr<TwitchEmoteSet>> newEmoteSets;

    {
        // unique_lock twitchEmoteSets
        for (const auto &emoteSetID : emoteSets)
        {
            if (!this->twitchEmoteSets.contains(emoteSetID))
            {
                auto emoteSet = std::make_shared<TwitchEmoteSet>();
                newEmoteSets.insert(emoteSetID, emoteSet);
                this->twitchEmoteSets.emplace(emoteSetID, emoteSet);
            }
        }
    }

    auto batches = splitListIntoBatches2(newEmoteSets);
    for (auto &&batch : batches)
    {
        const auto emoteSetIDs = batch.keys().join(',');
        getIvr()->getBulkEmoteSets(
            emoteSetIDs,
            [this, batch = std::move(batch)](QJsonArray emoteSetArray) {
                for (auto emoteSetResponse : emoteSetArray)
                {
                    IvrEmoteSet ivrEmoteSet(emoteSetResponse.toObject());
                    qDebug() << ivrEmoteSet.setId << ivrEmoteSet.displayName
                             << ivrEmoteSet.login << ivrEmoteSet.channelId
                             << ivrEmoteSet.tier << ivrEmoteSet.emotes;
                    auto emoteSet = batch[ivrEmoteSet.setId];
                    assert(emoteSet != nullptr);
                    emoteSet->ivrEmoteSet = ivrEmoteSet;

                    for (const auto &emoteObj : ivrEmoteSet.emotes)
                    {
                        IvrEmote ivrEmote(emoteObj.toObject());
                        if (emoteSet->emoteSetTypeString.isEmpty())
                        {
                            emoteSet->emoteSetTypeString = ivrEmote.emoteType;
                        }
                        if (!emoteSet->emoteSetType)
                        {
                            emoteSet->emoteSetType =
                                magic_enum::enum_cast<TwitchEmoteType>(
                                    ivrEmote.emoteType.toStdString());
                            if (emoteSet->emoteSetType)
                            {
                                qDebug() << "XXX: Set the emote set type to"
                                         << QString::fromUtf8(
                                                magic_enum::enum_name(
                                                    *emoteSet->emoteSetType)
                                                    .data());
                            }
                            else
                            {
                                qDebug()
                                    << "XXX: Failed to set emote set type: "
                                    << ivrEmote.emoteType;
                            }
                        }

                        auto id = EmoteId{ivrEmote.id};
                        auto code = EmoteName{
                            TwitchEmotes::cleanUpEmoteCode(ivrEmote.code)};

                        // emoteSet->emotes.push_back(TwitchEmote{id, code});

                        Emote::Type emoteType = Emote::Type::Twitch;

                        if (ivrEmoteSet.setId == "0")
                        {
                            emoteType = Emote::Type::TwitchGlobal;
                        }

                        // TODO: Should this be createOrUpdate?
                        auto emote = this->getOrCreateEmote(
                            id, code,
                            {
                                .type = emoteType,
                                .author = ivrEmoteSet.displayName,
                            });

                        // Follower emotes can be only used in their origin channel
                        emoteSet->emotes.emplace(code, emote);
                    }
                }
                qDebug() << "XXX: Emitting setsChanged signal";
                this->setsChanged();
            },
            [] {
                // fetching emotes failed, ivr API might be down
            });
    }
}

Url TwitchEmotes::getEmoteLink(const EmoteId &id, const QString &emoteScale)
{
    return {QString(TWITCH_EMOTE_TEMPLATE)
                .replace("{id}", id.string)
                .replace("{scale}", emoteScale)};
}

}  // namespace chatterino
