#include "providers/ffz/FfzEmoteProvider.hpp"

#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/ffz/FfzUtil.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringBuilder>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoFfzemotes;

const QString CHANNEL_HAS_NO_EMOTES(
    "This channel has no FrankerFaceZ channel emotes.");

const QString PROVIDER_ID = u"frankerfacez"_s;
const QString PROVIDER_NAME = u"FrankerFraceZ"_s;

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
        });
    }
    return authorityBadge;
}

FfzChannelBadgeMap parseFfzChannelBadges(const QJsonObject &badgeRoot)
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

}  // namespace

namespace chatterino {

FfzEmoteProvider::FfzEmoteProvider()
    : BuiltinEmoteProvider(&getSettings()->enableFFZGlobalEmotes,
                           u"https://api.frankerfacez.com/v1/set/global"_s,
                           &getSettings()->enableFFZChannelEmotes,
                           PROVIDER_NAME, PROVIDER_ID, FFZ_PRIORITY)
{
}

std::optional<EmoteMap> FfzEmoteProvider::parseChannelEmotes(
    TwitchChannel &twitch, const QJsonValue &json)
{
    auto jsonRoot = json.toObject();

    twitch.setFfzCustomModBadge(
        parseAuthorityBadge(json["room"]["mod_urls"].toObject(), "Moderator"));
    twitch.setFfzCustomVipBadge(
        parseAuthorityBadge(json["room"]["vip_badge"].toObject(), "VIP"));
    twitch.setFfzChannelBadges(
        parseFfzChannelBadges(json["room"]["user_badge_ids"].toObject()));

    auto emotes = EmoteMap();
    for (const auto emoteSetRef : jsonRoot["sets"].toObject())
    {
        parseEmoteSetInto(emoteSetRef.toObject(), "Channel", emotes);
    }
    return {std::move(emotes)};
}

std::optional<EmoteMap> FfzEmoteProvider::parseGlobalEmotes(
    const QJsonValue &json)
{
    auto jsonRoot = json.toObject();

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

        this->parseEmoteSetInto(emoteSet, "Global", emotes);
    }

    return {std::move(emotes)};
}

QString FfzEmoteProvider::channelEmotesUrl(const TwitchChannel &twitch) const
{
    return u"https://api.frankerfacez.com/v1/room/id/" % twitch.roomId();
}

void FfzEmoteProvider::parseEmoteSetInto(const QJsonObject &emoteSet,
                                         const QString &kind, EmoteMap &map)
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
        emote.homePage = Url{u"https://www.frankerfacez.com/emoticon/" %
                             id.string % '-' % name.string};
        emote.id = std::move(id);

        map[name] = this->createEmote(std::move(emote));
    }
}

}  // namespace chatterino
