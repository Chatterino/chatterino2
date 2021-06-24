#include "providers/seventv/SeventvEmotes.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino {
namespace {
    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no 7TV channel emotes.");

    QString emoteLinkFormat("https://7tv.app/emotes/%1");
    Url getEmoteLink(const EmoteId &id, const QString &emoteScale)

    {
        static const QString urlTemplate("https://cdn.7tv.app/emote/%1/%2");

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

        auto emote =
            Emote({name,
                   ImageSet{Image::fromUrl(getEmoteLink(id, "1x"), 1),
                            Image::fromUrl(getEmoteLink(id, "2x"), 0.66),
                            Image::fromUrl(getEmoteLink(id, "3x"), 0.33)},
                   Tooltip{QString("%1<br> %2 7TV Emote<br>By: %3")
                               .arg(name.string)
                               .arg(isGlobal ? "Global" : "Channel")
                               .arg(author.string)

                   },
                   Url{emoteLinkFormat.arg(id.string)}});

        auto result = CreateEmoteResult({id, name, emote});
        return result;
    }

    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonArray &jsonEmotes, const EmoteMap &currentEmotes)
    {
        auto emotes = EmoteMap();

        for (auto jsonEmote : jsonEmotes)
        {
            auto emote = createEmote(jsonEmote, true);
            emotes[emote.name] =
                cachedOrMakeEmotePtr(std::move(emote.emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }

    EmoteMap parseChannelEmotes(const QJsonObject &jsonRoot,
                                const QString &channelName)
    {
        auto emotes = EmoteMap();

        auto jsonEmotes = jsonRoot.value("emotes").toArray();
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
    // TODO: Network request
    qCDebug(chatterinoSeventv) << "[7TVEmotes] Loading Emotes";

    QJsonObject body;

    const char *gqlQuery = R""""(
        {
            search_emotes(query: "", globalState: "only", limit: 150, pageSize: 150) {
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
    )"""";

    body.insert("query", gqlQuery);
    body.insert("variables", QJsonObject{});

    QJsonDocument doc(body);
    QString str(doc.toJson(QJsonDocument::Compact));
    QByteArray b = QByteArray::fromStdString(str.toStdString());

    NetworkRequest(apiUrlGQL, NetworkRequestType::Post)
        .timeout(30000)
        .header("Content-Type", "application/json")
        .payload(b)
        .onSuccess([this](NetworkResult result) -> Outcome {
            QJsonArray parsedEmotes = result.parseJson()
                                          .value("data")
                                          .toObject()
                                          .value("search_emotes")
                                          .toArray();
            qCDebug(chatterinoSeventv) << "7TV Global Emotes:" << parsedEmotes;

            auto emotes = this->global_.get();
            auto pair = parseGlobalEmotes(parsedEmotes, *emotes);
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

void SeventvEmotes::loadChannel(std::weak_ptr<Channel> channel,
                                const QString &channelId,
                                const QString &channelLogin,
                                std::function<void(EmoteMap &&)> callback,
                                bool manualRefresh)
{
    qCDebug(chatterinoSeventv)
        << "[7TVEmotes] Reload 7TV Channel Emotes for channel" << channelId;

    QJsonObject body;

    std::string queryStr =
        R"(
        {
            user(id: "%1") {
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
        }
    )";
    QString query = QString::fromStdString(queryStr).arg(channelLogin);
    qCDebug(chatterinoSeventv) << query;

    body.insert("query", query);
    body.insert("variables", QJsonObject{});

    QJsonDocument doc(body);
    QString str(doc.toJson(QJsonDocument::Compact));
    QByteArray b = QByteArray::fromStdString(str.toStdString());

    NetworkRequest(apiUrlGQL, NetworkRequestType::Post)
        .timeout(20000)
        .header("Content-Type", "application/json")
        .payload(b)
        .onSuccess([callback = std::move(callback), channel, &channelId,
                    manualRefresh](NetworkResult result) -> Outcome {
            QJsonObject parsedEmotes = result.parseJson()
                                           .value("data")
                                           .toObject()
                                           .value("user")
                                           .toObject();
            qCDebug(chatterinoSeventv) << "7TV Channel Emotes:" << parsedEmotes;

            auto emoteMap = parseChannelEmotes(parsedEmotes, channelId);
            bool hasEmotes = !emoteMap.empty();

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
