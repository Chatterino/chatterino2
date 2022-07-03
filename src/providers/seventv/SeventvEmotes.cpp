#include "providers/seventv/SeventvEmotes.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>
#include <utility>

namespace chatterino {
namespace {
    const QRegularExpression whitespaceRegex(R"(\s+)");

    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no 7TV channel emotes.");
    const QString emoteLinkFormat("https://7tv.app/emotes/%1");

    // maximum pageSize that 7tv's API accepts
    constexpr int maxPageSize = 150;

    static std::unordered_map<EmoteId, std::weak_ptr<const Emote>> emoteCache;
    static std::mutex emoteCacheMutex;

    Url getEmoteLink(const EmoteId &id, const QString &emoteScale)

    {
        const QString urlTemplate("https://cdn.7tv.app/emote/%1/%2");

        return {urlTemplate.arg(id.string, emoteScale)};
    }

    EmotePtr cachedOrMake(Emote &&emote, const EmoteId &id)
    {
        return cachedOrMakeEmotePtr(std::move(emote), emoteCache,
                                    emoteCacheMutex, id);
    }

    struct CreateEmoteResult {
        EmoteId id;
        EmoteName name;
        Emote emote;
    };

    CreateEmoteResult createEmote(const QJsonValue &jsonEmote, bool isGlobal)
    {
        auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
        auto name = EmoteName{jsonEmote.toObject().value("name").toString()};
        auto author = EmoteAuthor{jsonEmote.toObject()
                                      .value("owner")
                                      .toObject()
                                      .value("display_name")
                                      .toString()};
        int64_t visibility = jsonEmote.toObject().value("visibility").toInt();
        auto visibilityFlags =
            SeventvEmoteVisibilityFlags(SeventvEmoteVisibilityFlag(visibility));
        bool zeroWidth =
            visibilityFlags.has(SeventvEmoteVisibilityFlag::ZeroWidth);

        auto heightArr = jsonEmote.toObject().value("height").toArray();

        auto size1x = heightArr.at(0).toDouble();
        auto size2x = size1x * 2;
        auto size3x = size1x * 3;
        auto size4x = size1x * 4;

        if (heightArr.size() >= 2)
        {
            size2x = heightArr.at(1).toDouble();
        }
        if (heightArr.size() >= 3)
        {
            size3x = heightArr.at(2).toDouble();
        }
        if (heightArr.size() >= 4)
        {
            size4x = heightArr.at(3).toDouble();
        }

        auto emote = Emote(
            {name,
             ImageSet{Image::fromUrl(getEmoteLink(id, "1x"), size1x / size1x),
                      Image::fromUrl(getEmoteLink(id, "2x"), size1x / size2x),
                      Image::fromUrl(getEmoteLink(id, "3x"), size1x / size3x),
                      Image::fromUrl(getEmoteLink(id, "4x"), size1x / size4x)},
             Tooltip{QString("%1<br>%2 7TV Emote<br>By: %3")
                         .arg(name.string, (isGlobal ? "Global" : "Channel"),
                              author.string)},
             Url{emoteLinkFormat.arg(id.string)}, zeroWidth});

        auto result = CreateEmoteResult({id, name, emote});
        return result;
    }

    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonArray &jsonEmotes, const EmoteMap &currentEmotes)
    {
        auto emotes = EmoteMap();

        // We always show all global emotes, no need to check visibility here
        for (const auto &jsonEmote : jsonEmotes)
        {
            auto emote = createEmote(jsonEmote, true);
            emotes[emote.name] =
                cachedOrMakeEmotePtr(std::move(emote.emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }

    bool checkEmoteVisibility(const QJsonObject &emoteJson)
    {
        int64_t visibility = emoteJson.value("visibility").toInt();
        auto visibilityFlags =
            SeventvEmoteVisibilityFlags(SeventvEmoteVisibilityFlag(visibility));
        return !visibilityFlags.has(SeventvEmoteVisibilityFlag::Unlisted) ||
               getSettings()->showUnlistedEmotes;
    }

    EmoteMap parseChannelEmotes(const QJsonObject &root,
                                const QString &channelName)
    {
        auto emotes = EmoteMap();

        auto jsonEmotes = root.value("emotes").toArray();
        for (auto jsonEmote_ : jsonEmotes)
        {
            auto jsonEmote = jsonEmote_.toObject();

            if (!checkEmoteVisibility(jsonEmote))
            {
                continue;
            }

            auto emote = createEmote(jsonEmote, false);
            emotes[emote.name] = cachedOrMake(std::move(emote.emote), emote.id);
        }

        return emotes;
    }

    void updateEmoteMapPtr(Atomic<std::shared_ptr<const EmoteMap>> &map,
                           EmoteMap &&updatedMap)
    {
        map.set(std::make_shared<EmoteMap>(std::move(updatedMap)));
    }

    boost::optional<EmotePtr> findEmote(
        const std::shared_ptr<const EmoteMap> &map,
        const QString &emoteBaseName, const QJsonObject &emoteJson)
    {
        auto id = emoteJson["id"].toString();

        // Step 1: check if the emote is added with the base name
        auto mapIt = map->find(EmoteName{emoteBaseName});
        // We still need to check for the id!
        if (mapIt != map->end() && mapIt->second->homePage.string.endsWith(id))
        {
            return mapIt->second;
        }

        std::lock_guard<std::mutex> guard(emoteCacheMutex);

        // Step 2: check the cache for the emote
        auto emote = emoteCache[EmoteId{id}].lock();
        if (emote)
        {
            auto cacheIt = map->find(emote->name);
            // Same as above, make sure it's actually the correct emote
            if (cacheIt != map->end() &&
                cacheIt->second->homePage.string.endsWith(id))
            {
                return cacheIt->second;
            }
        }
        // Step 3: Since the emote is not added, and it's not even in the cache,
        //         we need to check if the emote is added.
        //         This is expensive but the cache entry may be corrupted
        //         when an emote was added with a different alias in some other
        //         channel.
        for (const auto &[_, value] : *map)
        {
            // since the url ends with the emote id we can check this
            if (value->homePage.string.endsWith(id))
            {
                return value;
            }
        }
        return boost::none;
    }

}  // namespace

SeventvEmotes::SeventvEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> SeventvEmotes::emotes() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> SeventvEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

void SeventvEmotes::loadEmotes()
{
    qCDebug(chatterinoSeventv) << "Loading 7TV Emotes";

    QJsonObject payload, variables;

    QString query = R"(
        query loadGlobalEmotes($query: String!, $globalState: String, $page: Int, $limit: Int, $pageSize: Int) {
        search_emotes(query: $query, globalState: $globalState, page: $page, limit: $limit, pageSize: $pageSize) {
            id
            name
            provider
            provider_id
            visibility
            mime
            height
            owner {
                id
                display_name
                login
                twitch_id
            }
        }
    })";

    variables.insert("query", QString());
    variables.insert("globalState", "only");
    variables.insert("page", 1);  // TODO(zneix): Add support for pagination
    variables.insert("limit", maxPageSize);
    variables.insert("pageSize", maxPageSize);

    payload.insert("query", query.replace(whitespaceRegex, " "));
    payload.insert("variables", variables);

    NetworkRequest(apiUrlGQL, NetworkRequestType::Post)
        .timeout(30000)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([this](NetworkResult result) -> Outcome {
            QJsonArray parsedEmotes = result.parseJson()
                                          .value("data")
                                          .toObject()
                                          .value("search_emotes")
                                          .toArray();
            qCDebug(chatterinoSeventv)
                << "7TV Global Emotes" << parsedEmotes.size();

            auto pair = parseGlobalEmotes(parsedEmotes, *this->global_.get());
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

boost::optional<EmotePtr> SeventvEmotes::addEmote(
    Atomic<std::shared_ptr<const EmoteMap>> &map, const QJsonValue &emoteJson)
{
    // Check for visibility first, so we don't copy the map.
    if (!checkEmoteVisibility(emoteJson.toObject()))
    {
        return boost::none;
    }

    EmoteMap updatedMap = *map.get();
    auto emote = createEmote(emoteJson, false);
    auto emotePtr = cachedOrMake(std::move(emote.emote), emote.id);
    updatedMap[emote.name] = emotePtr;
    updateEmoteMapPtr(map, std::move(updatedMap));

    return emotePtr;
}

boost::optional<EmotePtr> SeventvEmotes::updateEmote(
    Atomic<std::shared_ptr<const EmoteMap>> &map, QString *emoteBaseName,
    const QJsonValue &emoteJson)
{
    auto oldMap = map.get();
    auto foundEmote = findEmote(oldMap, *emoteBaseName, emoteJson.toObject());
    if (!foundEmote)
    {
        return boost::none;
    }

    *emoteBaseName = foundEmote->get()->getCopyString();

    EmoteMap updatedMap = *map.get();
    updatedMap.erase(foundEmote.value()->name);

    auto emote = createEmote(emoteJson, false);
    auto emotePtr = cachedOrMake(std::move(emote.emote), emote.id);
    updatedMap[emote.name] = emotePtr;
    updateEmoteMapPtr(map, std::move(updatedMap));

    return emotePtr;
}

bool SeventvEmotes::removeEmote(Atomic<std::shared_ptr<const EmoteMap>> &map,
                                const QString &emoteName)
{
    EmoteMap updatedMap = *map.get();
    auto it = updatedMap.find(EmoteName{emoteName});
    if (it == updatedMap.end())
    {
        // We already copied the map at this point and are now discarding the copy.
        // This is fine, because this case should be really rare.
        return false;
    }
    updatedMap.erase(it);
    updateEmoteMapPtr(map, std::move(updatedMap));
    return true;
}

void SeventvEmotes::loadChannel(std::weak_ptr<Channel> channel,
                                const QString &channelId,
                                std::function<void(EmoteMap &&)> callback,
                                bool manualRefresh)
{
    qCDebug(chatterinoSeventv)
        << "Reloading 7TV Channel Emotes" << channelId << manualRefresh;

    QJsonObject payload, variables;

    QString query = R"(
        query loadUserEmotes($login: String!) {
            user(id: $login) {
                emotes {
                    id
                    name
                    provider
                    provider_id
                    visibility
                    mime
                    height
                    owner {
                        id
                        display_name
                        login
                        twitch_id
                    }
                }
            }
        })";

    variables.insert("login", channelId);

    payload.insert("query", query.replace(whitespaceRegex, " "));
    payload.insert("variables", variables);

    qDebug() << QJsonDocument(payload).toJson(QJsonDocument::Compact);

    NetworkRequest(apiUrlGQL, NetworkRequestType::Post)
        .timeout(20000)
        .header("Content-Type", "application/json")
        .payload(QJsonDocument(payload).toJson(QJsonDocument::Compact))
        .onSuccess([callback = std::move(callback), channel, channelId,
                    manualRefresh](NetworkResult result) -> Outcome {
            QJsonObject parsedEmotes = result.parseJson()
                                           .value("data")
                                           .toObject()
                                           .value("user")
                                           .toObject();

            auto emoteMap = parseChannelEmotes(parsedEmotes, channelId);
            bool hasEmotes = !emoteMap.empty();

            qCDebug(chatterinoSeventv)
                << "Loaded 7TV Channel Emotes" << channelId << emoteMap.size()
                << manualRefresh;

            if (hasEmotes)
            {
                callback(std::move(emoteMap));
            }
            if (auto shared = channel.lock(); manualRefresh)
            {
                if (hasEmotes)
                {
                    shared->addMessage(
                        makeSystemMessage("7TV channel emotes reloaded."));
                }
                else
                {
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                }
            }
            return Success;
        })
        .onError([channelId, channel, manualRefresh](NetworkResult result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 400)
            {
                qCWarning(chatterinoSeventv)
                    << "Error occured fetching 7TV emotes: "
                    << result.parseJson();
                if (manualRefresh)
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
            }
            else if (result.status() == NetworkResult::timedoutStatus)
            {
                // TODO: Auto retry in case of a timeout, with a delay
                qCWarning(chatterinoSeventv())
                    << "Fetching 7TV emotes for channel" << channelId
                    << "failed due to timeout";
                shared->addMessage(makeSystemMessage(
                    "Failed to fetch 7TV channel emotes. (timed out)"));
            }
            else
            {
                qCWarning(chatterinoSeventv)
                    << "Error fetching 7TV emotes for channel" << channelId
                    << ", error" << result.status();
                shared->addMessage(
                    makeSystemMessage("Failed to fetch 7TV channel "
                                      "emotes. (unknown error)"));
            }
        })
        .execute();
}

}  // namespace chatterino
