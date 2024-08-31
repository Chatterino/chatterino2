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

namespace {

using namespace chatterino;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoFfzemotes;

const QString CHANNEL_HAS_NO_EMOTES(
    "This channel has no FrankerFaceZ channel emotes.");

// FFZ doesn't provide any data on the size for room badges,
// so we assume 18x18 (same as a Twitch badge)
constexpr QSize BASE_BADGE_SIZE(18, 18);

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

void fillInEmoteData(const QJsonObject &emote, const QJsonObject &urls,
                     const EmoteName &name, const QString &tooltip,
                     Emote &emoteData)
{
    auto url1x = getEmoteLink(urls, "1");
    auto url2x = getEmoteLink(urls, "2");
    auto url3x = getEmoteLink(urls, "4");
    QSize baseSize(emote["width"].toInt(28), emote["height"].toInt(28));

    //, code, tooltip
    emoteData.name = name;
    emoteData.images = ImageSet{
        Image::fromUrl(url1x, 1, baseSize),
        url2x.string.isEmpty() ? Image::getEmpty()
                               : Image::fromUrl(url2x, 0.5, baseSize * 2),
        url3x.string.isEmpty() ? Image::getEmpty()
                               : Image::fromUrl(url3x, 0.25, baseSize * 4)};
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
        fillInEmoteData(emoteJson, urls, name,
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
            qCDebug(LOG) << "Skipping global emote set" << emoteSetID
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
            Image::fromUrl(authorityBadge1x, 1, BASE_BADGE_SIZE),
            authorityBadge2x.string.isEmpty()
                ? Image::getEmpty()
                : Image::fromUrl(authorityBadge2x, 0.5, BASE_BADGE_SIZE * 2),
            authorityBadge3x.string.isEmpty()
                ? Image::getEmpty()
                : Image::fromUrl(authorityBadge3x, 0.25, BASE_BADGE_SIZE * 4),
        };

        authorityBadge = std::make_shared<Emote>(Emote{
            .name = {""},
            .images = authorityBadgeImageSet,
            .tooltip = Tooltip{tooltip},
            .homePage = authorityBadge1x,
        });
    }
    return authorityBadge;
}

}  // namespace

namespace chatterino {

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

FfzChannelBadgeMap ffz::detail::parseChannelBadges(const QJsonObject &badgeRoot)
{
    FfzChannelBadgeMap channelBadges;

    for (auto it = badgeRoot.begin(); it != badgeRoot.end(); ++it)
    {
        const auto badgeID = it.key().toInt();
        const auto &jsonUserIDs = it.value().toArray();
        for (const auto &jsonUserID : jsonUserIDs)
        {
            // NOTE: The Twitch User IDs come through as ints right now, the code below
            // tries to parse them as strings first since that's how we treat them anyway.
            if (jsonUserID.isString())
            {
                channelBadges[jsonUserID.toString()].emplace_back(badgeID);
            }
            else
            {
                channelBadges[QString::number(jsonUserID.toInt())].emplace_back(
                    badgeID);
            }
        }
    }

    return channelBadges;
}

FfzEmotes::FfzEmotes()
    : global_(std::make_shared<EmoteMap>())
{
    getSettings()->enableFFZGlobalEmotes.connect(
        [this] {
            this->loadEmotes();
        },
        this->managedConnections, false);
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
    std::function<void(FfzChannelBadgeMap &&)> channelBadgesCallback,
    bool manualRefresh)
{
    qCDebug(LOG) << "Reload FFZ Channel Emotes for channel" << channelID;

    NetworkRequest("https://api.frankerfacez.com/v1/room/id/" + channelID)

        .timeout(20000)
        .onSuccess([emoteCallback = std::move(emoteCallback),
                    modBadgeCallback = std::move(modBadgeCallback),
                    vipBadgeCallback = std::move(vipBadgeCallback),
                    channelBadgesCallback = std::move(channelBadgesCallback),
                    channel, manualRefresh](const auto &result) {
            const auto json = result.parseJson();

            auto emoteMap = parseChannelEmotes(json);
            auto modBadge = parseAuthorityBadge(
                json["room"]["mod_urls"].toObject(), "Moderator");
            auto vipBadge = parseAuthorityBadge(
                json["room"]["vip_badge"].toObject(), "VIP");
            auto channelBadges =
                parseChannelBadges(json["room"]["user_badge_ids"].toObject());

            bool hasEmotes = !emoteMap.empty();

            emoteCallback(std::move(emoteMap));
            modBadgeCallback(std::move(modBadge));
            vipBadgeCallback(std::move(vipBadge));
            channelBadgesCallback(std::move(channelBadges));
            if (auto shared = channel.lock(); manualRefresh)
            {
                if (hasEmotes)
                {
                    shared->addSystemMessage(
                        "FrankerFaceZ channel emotes reloaded.");
                }
                else
                {
                    shared->addSystemMessage(CHANNEL_HAS_NO_EMOTES);
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
                    shared->addSystemMessage(CHANNEL_HAS_NO_EMOTES);
                }
            }
            else
            {
                // TODO: Auto retry in case of a timeout, with a delay
                auto errorString = result.formatError();
                qCWarning(LOG) << "Error fetching FFZ emotes for channel"
                               << channelID << ", error" << errorString;
                shared->addSystemMessage(
                    QStringLiteral("Failed to fetch FrankerFaceZ channel "
                                   "emotes. (Error: %1)")
                        .arg(errorString));
            }
        })
        .execute();
}

}  // namespace chatterino
