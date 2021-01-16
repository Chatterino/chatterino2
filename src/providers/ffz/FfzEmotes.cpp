#include "providers/ffz/FfzEmotes.hpp"

#include <QJsonArray>

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino {
namespace {
    Url getEmoteLink(const QJsonObject &urls, const QString &emoteScale)
    {
        auto emote = urls.value(emoteScale);
        if (emote.isUndefined() || emote.isNull())
        {
            return {""};
        }

        assert(emote.isString());

        return {"https:" + emote.toString()};
    }
    void fillInEmoteData(const QJsonObject &urls, const EmoteName &name,
                         const QString &tooltip, Emote &emoteData)
    {
        auto url1x = getEmoteLink(urls, "1");
        auto url2x = getEmoteLink(urls, "2");
        auto url3x = getEmoteLink(urls, "4");

        //, code, tooltip
        emoteData.name = name;
        emoteData.images =
            ImageSet{Image::fromUrl(url1x, 1),
                     url2x.string.isEmpty() ? Image::getEmpty()
                                            : Image::fromUrl(url2x, 0.5),
                     url3x.string.isEmpty() ? Image::getEmpty()
                                            : Image::fromUrl(url3x, 0.25)};
        emoteData.tooltip = {tooltip};
    }
    EmotePtr cachedOrMake(Emote &&emote, const EmoteId &id)
    {
        static std::unordered_map<EmoteId, std::weak_ptr<const Emote>> cache;
        static std::mutex mutex;

        return cachedOrMakeEmotePtr(std::move(emote), cache, mutex, id);
    }
    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonObject &jsonRoot, const EmoteMap &currentEmotes)
    {
        auto jsonSets = jsonRoot.value("sets").toObject();
        auto emotes = EmoteMap();

        for (auto jsonSet : jsonSets)
        {
            auto jsonEmotes = jsonSet.toObject().value("emoticons").toArray();

            for (auto jsonEmoteValue : jsonEmotes)
            {
                auto jsonEmote = jsonEmoteValue.toObject();

                auto name = EmoteName{jsonEmote.value("name").toString()};
                auto id = EmoteId{jsonEmote.value("id").toString()};
                auto urls = jsonEmote.value("urls").toObject();

                auto emote = Emote();
                fillInEmoteData(urls, name,
                                name.string + "<br>Global FFZ Emote", emote);
                emote.homePage =
                    Url{QString("https://www.frankerfacez.com/emoticon/%1-%2")
                            .arg(id.string)
                            .arg(name.string)};

                emotes[name] =
                    cachedOrMakeEmotePtr(std::move(emote), currentEmotes);
            }
        }

        return {Success, std::move(emotes)};
    }

    boost::optional<EmotePtr> parseModBadge(const QJsonObject &jsonRoot)
    {
        boost::optional<EmotePtr> modBadge;

        auto room = jsonRoot.value("room").toObject();
        auto modUrls = room.value("mod_urls").toObject();
        if (!modUrls.isEmpty())
        {
            auto modBadge1x = getEmoteLink(modUrls, "1");
            auto modBadge2x = getEmoteLink(modUrls, "2");
            auto modBadge3x = getEmoteLink(modUrls, "4");

            auto modBadgeImageSet = ImageSet{
                Image::fromUrl(modBadge1x, 1),
                modBadge2x.string.isEmpty() ? Image::getEmpty()
                                            : Image::fromUrl(modBadge2x, 0.5),
                modBadge3x.string.isEmpty() ? Image::getEmpty()
                                            : Image::fromUrl(modBadge3x, 0.25),
            };

            modBadge = std::make_shared<Emote>(Emote{
                {""},
                modBadgeImageSet,
                Tooltip{"Moderator"},
                modBadge1x,
            });
        }
        return modBadge;
    }

    EmoteMap parseChannelEmotes(const QJsonObject &jsonRoot)
    {
        auto jsonSets = jsonRoot.value("sets").toObject();
        auto emotes = EmoteMap();

        for (auto jsonSet : jsonSets)
        {
            auto jsonEmotes = jsonSet.toObject().value("emoticons").toArray();

            for (auto _jsonEmote : jsonEmotes)
            {
                auto jsonEmote = _jsonEmote.toObject();

                // margins
                auto id =
                    EmoteId{QString::number(jsonEmote.value("id").toInt())};
                auto name = EmoteName{jsonEmote.value("name").toString()};
                auto author = EmoteAuthor{jsonEmote.value("owner")
                                              .toObject()
                                              .value("display_name")
                                              .toString()};
                auto urls = jsonEmote.value("urls").toObject();

                Emote emote;
                fillInEmoteData(urls, name,
                                QString("%1<br>Channel FFZ Emote<br>By: %2")
                                    .arg(name.string)
                                    .arg(author.string),
                                emote);
                emote.homePage =
                    Url{QString("https://www.frankerfacez.com/emoticon/%1-%2")
                            .arg(id.string)
                            .arg(name.string)};

                emotes[name] = cachedOrMake(std::move(emote), id);
            }
        }

        return emotes;
    }
}  // namespace

FfzEmotes::FfzEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> FfzEmotes::emotes() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> FfzEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);
    if (it != emotes->end())
        return it->second;
    return boost::none;
}

void FfzEmotes::loadEmotes()
{
    QString url("https://api.frankerfacez.com/v1/set/global");

    NetworkRequest(url)

        .timeout(30000)
        .onSuccess([this](auto result) -> Outcome {
            auto emotes = this->emotes();
            auto pair = parseGlobalEmotes(result.parseJson(), *emotes);
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

void FfzEmotes::loadChannel(
    std::weak_ptr<Channel> channel, const QString &channelId,
    std::function<void(EmoteMap &&)> emoteCallback,
    std::function<void(boost::optional<EmotePtr>)> modBadgeCallback,
    bool manualRefresh)
{
    qCDebug(chatterinoFfzemotes)
        << "[FFZEmotes] Reload FFZ Channel Emotes for channel" << channelId;

    NetworkRequest("https://api.frankerfacez.com/v1/room/id/" + channelId)

        .timeout(20000)
        .onSuccess([emoteCallback = std::move(emoteCallback),
                    modBadgeCallback = std::move(modBadgeCallback), channel,
                    manualRefresh](auto result) -> Outcome {
            auto json = result.parseJson();
            auto emoteMap = parseChannelEmotes(json);
            auto modBadge = parseModBadge(json);

            emoteCallback(std::move(emoteMap));
            modBadgeCallback(std::move(modBadge));
            if (auto shared = channel.lock(); manualRefresh)
                shared->addMessage(
                    makeSystemMessage("FrankerFaceZ channel emotes reloaded."));

            return Success;
        })
        .onError([channelId, channel, manualRefresh](NetworkResult result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 203)
            {
                // User does not have any FFZ emotes
                if (manualRefresh)
                    shared->addMessage(makeSystemMessage(
                        "This channel has no FrankerFaceZ channel emotes."));
            }
            else if (result.status() == NetworkResult::timedoutStatus)
            {
                // TODO: Auto retry in case of a timeout, with a delay
                qCWarning(chatterinoFfzemotes)
                    << "Fetching FFZ emotes for channel" << channelId
                    << "failed due to timeout";
                shared->addMessage(
                    makeSystemMessage("Failed to fetch FrankerFaceZ channel "
                                      "emotes. (timed out)"));
            }
            else
            {
                qCWarning(chatterinoFfzemotes)
                    << "Error fetching FFZ emotes for channel" << channelId
                    << ", error" << result.status();
                shared->addMessage(
                    makeSystemMessage("Failed to fetch FrankerFaceZ channel "
                                      "emotes. (unknown error)"));
            }
        })
        .execute();
}

}  // namespace chatterino
