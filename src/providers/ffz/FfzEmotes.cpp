#include "providers/ffz/FfzEmotes.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/ffz/FfzUtil.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {
namespace {

    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no FrankerFaceZ channel emotes.");

    Url getEmoteLink(const QJsonObject &urls, const QString &emoteScale)
    {
        auto emote = urls[emoteScale];
        if (emote.isUndefined() || emote.isNull())
        {
            return {""};
        }

        assert(emote.isString());

        return parseFfzUrl(emote.toString());
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

    void parseEmoteSetInto(const QJsonObject &emoteSet, const QString &kind,
                           EmoteMap &map)
    {
        for (const auto emoteRef : emoteSet["emoticons"].toArray())
        {
            const auto emoteJson = emoteRef.toObject();

            // margins
            auto id = EmoteId{QString::number(emoteJson["id"].toInt())};
            auto name = EmoteName{emoteJson["name"].toString()};
            auto author =
                EmoteAuthor{emoteJson["owner"]["display_name"].toString()};
            auto urls = emoteJson["urls"].toObject();
            if (emoteJson["animated"].isObject())
            {
                // prefer animated images if available
                urls = emoteJson["animated"].toObject();
            }

            Emote emote;
            fillInEmoteData(urls, name,
                            QString("%1<br>%2 FFZ Emote<br>By: %3")
                                .arg(name.string, kind, author.string),
                            emote);
            emote.homePage =
                Url{QString("https://www.frankerfacez.com/emoticon/%1-%2")
                        .arg(id.string)
                        .arg(name.string)};

            map[name] = cachedOrMake(std::move(emote), id);
        }
    }

    EmoteMap parseGlobalEmotes(const QJsonObject &jsonRoot)
    {
        // Load default sets from the `default_sets` object
        std::unordered_set<int> defaultSets{};
        auto jsonDefaultSets = jsonRoot["default_sets"].toArray();
        for (auto jsonDefaultSet : jsonDefaultSets)
        {
            defaultSets.insert(jsonDefaultSet.toInt());
        }

        auto emotes = EmoteMap();

        for (const auto emoteSetRef : jsonRoot["sets"].toObject())
        {
            const auto emoteSet = emoteSetRef.toObject();
            auto emoteSetID = emoteSet["id"].toInt();
            if (!defaultSets.contains(emoteSetID))
            {
                qCDebug(chatterinoFfzemotes)
                    << "Skipping global emote set" << emoteSetID
                    << "as it's not part of the default sets";
                continue;
            }

            parseEmoteSetInto(emoteSet, "Global", emotes);
        }

        return emotes;
    }

    std::optional<EmotePtr> parseAuthorityBadge(const QJsonObject &badgeUrls,
                                                const QString &tooltip)
    {
        std::optional<EmotePtr> authorityBadge;

        if (!badgeUrls.isEmpty())
        {
            auto authorityBadge1x = getEmoteLink(badgeUrls, "1");
            auto authorityBadge2x = getEmoteLink(badgeUrls, "2");
            auto authorityBadge3x = getEmoteLink(badgeUrls, "4");

            auto authorityBadgeImageSet = ImageSet{
                Image::fromUrl(authorityBadge1x, 1),
                authorityBadge2x.string.isEmpty()
                    ? Image::getEmpty()
                    : Image::fromUrl(authorityBadge2x, 0.5),
                authorityBadge3x.string.isEmpty()
                    ? Image::getEmpty()
                    : Image::fromUrl(authorityBadge3x, 0.25),
            };

            authorityBadge = std::make_shared<Emote>(Emote{
                {""},
                authorityBadgeImageSet,
                Tooltip{tooltip},
                authorityBadge1x,
            });
        }
        return authorityBadge;
    }

}  // namespace

using namespace ffz::detail;

EmoteMap ffz::detail::parseChannelEmotes(const QJsonObject &jsonRoot)
{
    auto emotes = EmoteMap();

    for (const auto emoteSetRef : jsonRoot["sets"].toObject())
    {
        parseEmoteSetInto(emoteSetRef.toObject(), "Channel", emotes);
    }

    return emotes;
}

FfzEmotes::FfzEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> FfzEmotes::emotes() const
{
    return this->global_.get();
}

std::optional<EmotePtr> FfzEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);
    if (it != emotes->end())
    {
        return it->second;
    }
    return std::nullopt;
}

void FfzEmotes::loadEmotes()
{
    if (!Settings::instance().enableFFZGlobalEmotes)
    {
        this->setEmotes(EMPTY_EMOTE_MAP);
        return;
    }

    QString url("https://api.frankerfacez.com/v1/set/global");

    NetworkRequest(url)

        .timeout(30000)
        .onSuccess([this](auto result) {
            auto parsedSet = parseGlobalEmotes(result.parseJson());
            this->setEmotes(std::make_shared<EmoteMap>(std::move(parsedSet)));
        })
        .execute();
}

void FfzEmotes::setEmotes(std::shared_ptr<const EmoteMap> emotes)
{
    this->global_.set(std::move(emotes));
}

void FfzEmotes::loadChannel(
    std::weak_ptr<Channel> channel, const QString &channelID,
    std::function<void(EmoteMap &&)> emoteCallback,
    std::function<void(std::optional<EmotePtr>)> modBadgeCallback,
    std::function<void(std::optional<EmotePtr>)> vipBadgeCallback,
    bool manualRefresh)
{
    qCDebug(chatterinoFfzemotes)
        << "[FFZEmotes] Reload FFZ Channel Emotes for channel" << channelID;

    NetworkRequest("https://api.frankerfacez.com/v1/room/id/" + channelID)

        .timeout(20000)
        .onSuccess([emoteCallback = std::move(emoteCallback),
                    modBadgeCallback = std::move(modBadgeCallback),
                    vipBadgeCallback = std::move(vipBadgeCallback), channel,
                    manualRefresh](const auto &result) {
            const auto json = result.parseJson();

            auto emoteMap = parseChannelEmotes(json);
            auto modBadge = parseAuthorityBadge(
                json["room"]["mod_urls"].toObject(), "Moderator");
            auto vipBadge = parseAuthorityBadge(
                json["room"]["vip_badge"].toObject(), "VIP");

            bool hasEmotes = !emoteMap.empty();

            emoteCallback(std::move(emoteMap));
            modBadgeCallback(std::move(modBadge));
            vipBadgeCallback(std::move(vipBadge));
            if (auto shared = channel.lock(); manualRefresh)
            {
                if (hasEmotes)
                {
                    shared->addMessage(makeSystemMessage(
                        "FrankerFaceZ channel emotes reloaded."));
                }
                else
                {
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                }
            }
        })
        .onError([channelID, channel, manualRefresh](const auto &result) {
            auto shared = channel.lock();
            if (!shared)
            {
                return;
            }

            if (result.status() == 404)
            {
                // User does not have any FFZ emotes
                if (manualRefresh)
                {
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                }
            }
            else
            {
                // TODO: Auto retry in case of a timeout, with a delay
                auto errorString = result.formatError();
                qCWarning(chatterinoFfzemotes)
                    << "Error fetching FFZ emotes for channel" << channelID
                    << ", error" << errorString;
                shared->addMessage(makeSystemMessage(
                    QStringLiteral("Failed to fetch FrankerFaceZ channel "
                                   "emotes. (Error: %1)")
                        .arg(errorString)));
            }
        })
        .execute();
}

}  // namespace chatterino
