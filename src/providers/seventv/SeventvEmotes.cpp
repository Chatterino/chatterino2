#include "providers/seventv/SeventvEmotes.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>

namespace chatterino {
namespace {
    const QRegularExpression whitespaceRegex(R"(\s+)");

    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no 7TV channel emotes.");
    const QString emoteLinkFormat("https://7tv.app/emotes/%1");

    // maximum pageSize that 7tv's API accepts
    constexpr int maxPageSize = 150;

    Url getEmoteLink(const EmoteId &id, const QString &emoteScale)

    {
        const QString urlTemplate("https://cdn.7tv.app/emote/%1/%2");

        return {urlTemplate.arg(id.string, emoteScale)};
    }

    EmotePtr cachedOrMake(Emote &&emote, const EmoteId &id)
    {
        static std::unordered_map<EmoteId, std::weak_ptr<const Emote>> cache;
        static std::mutex mutex;

        return cachedOrMakeEmotePtr(std::move(emote), cache, mutex, id);
    }

    struct CreateEmoteResult {
        EmoteId id;
        EmoteName name;
        Emote emote;
    };

    CreateEmoteResult createEmote(QJsonValue jsonEmote, bool isGlobal)
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

        auto emote = Emote(
            {name,
             ImageSet{Image::fromUrl(getEmoteLink(id, "1x"), 1),
                      Image::fromUrl(getEmoteLink(id, "2x"), 0.66),
                      Image::fromUrl(getEmoteLink(id, "3x"), 0.33)},
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

        for (const auto &jsonEmote : jsonEmotes)
        {
            auto emote = createEmote(jsonEmote, true);
            emotes[emote.name] =
                cachedOrMakeEmotePtr(std::move(emote.emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }

    EmoteMap parseChannelEmotes(const QJsonObject &root,
                                const QString &channelName)
    {
        auto emotes = EmoteMap();

        auto jsonEmotes = root.value("emotes").toArray();
        for (auto jsonEmote_ : jsonEmotes)
        {
            auto jsonEmote = jsonEmote_.toObject();

            auto emote = createEmote(jsonEmote, false);

            emotes[emote.name] = cachedOrMake(std::move(emote.emote), emote.id);
        }

        return emotes;
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
