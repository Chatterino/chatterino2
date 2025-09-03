#include "providers/bttv/BttvEmoteProvider.hpp"

#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringBuilder>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

const QString CHANNEL_HAS_NO_EMOTES(
    "This channel has no BetterTTV channel emotes.");

const QString PROVIDER_ID = u"bttv"_s;
const QString PROVIDER_NAME = u"BetterTTV"_s;

/// The emote page template.
///
/// %1 being the emote ID (e.g. 566ca04265dbbdab32ec054a)
constexpr QStringView EMOTE_LINK_FORMAT = u"https://betterttv.com/emotes/%1";

/// The emote CDN link template.
///
/// %1 being the emote ID (e.g. 566ca04265dbbdab32ec054a)
///
/// %2 being the emote size (e.g. 3x)
constexpr QStringView EMOTE_CDN_FORMAT =
    u"https://cdn.betterttv.net/emote/%1/%2.webp";

const QString GLOBAL_EMOTE_API_URL =
    u"https://api.betterttv.net/3/cached/emotes/global"_s;
constexpr QStringView CHANNEL_EMOTE_API_URL =
    u"https://api.betterttv.net/3/cached/users/twitch/";

// BTTV doesn't provide any data on the size, so we assume an emote is 28x28
constexpr QSize EMOTE_BASE_SIZE(28, 28);

const QSet<QString> ZERO_WIDTH_GLOBALS{
    "SoSnowy",  "IceCold",   "SantaHat", "TopHat",
    "ReinDeer", "CandyCane", "cvMask",   "cvHazmat",
};

struct CreateEmoteResult {
    EmoteId id;
    EmoteName name;
    Emote emote;
};

Url getEmoteLinkV3(const EmoteId &id, const QString &emoteScale)
{
    return {EMOTE_CDN_FORMAT.arg(id.string, emoteScale)};
}

CreateEmoteResult createChannelEmote(const QString &channelDisplayName,
                                     const QJsonObject &jsonEmote)
{
    auto id = EmoteId{jsonEmote.value("id").toString()};
    auto name = EmoteName{jsonEmote.value("code").toString()};
    auto author = EmoteAuthor{
        jsonEmote.value("user").toObject().value("displayName").toString()};
    if (author.string.isEmpty())
    {
        author.string = jsonEmote["channel"].toString();
    }

    auto emote = Emote({
        .name = name,
        .images =
            ImageSet{
                Image::fromUrl(getEmoteLinkV3(id, "1x"), 1, EMOTE_BASE_SIZE),
                Image::fromUrl(getEmoteLinkV3(id, "2x"), 0.5,
                               EMOTE_BASE_SIZE * 2),
                Image::fromUrl(getEmoteLinkV3(id, "3x"), 0.25,
                               EMOTE_BASE_SIZE * 4),
            },
        .tooltip =
            Tooltip{
                QString("%1<br>%2 BetterTTV Emote<br>By: %3")
                    .arg(name.string)
                    // when author is empty, it is a channel emote created by the broadcaster
                    .arg(author.string.isEmpty() ? "Channel" : "Shared")
                    .arg(author.string.isEmpty() ? channelDisplayName
                                                 : author.string)},
        .providerID = PROVIDER_ID,
        .zeroWidth = false,
        .id = id,
        .author = {},
        .baseName = {},
    });

    return {id, name, emote};
}

bool updateBttvChannelEmote(Emote &emote, const QString &channelDisplayName,
                            const QJsonObject &jsonEmote)
{
    bool anyModifications = false;

    if (jsonEmote.contains("code"))
    {
        emote.name = EmoteName{jsonEmote.value("code").toString()};
        anyModifications = true;
    }
    if (jsonEmote.contains("user"))
    {
        emote.author = EmoteAuthor{
            jsonEmote.value("user").toObject().value("displayName").toString()};
        anyModifications = true;
    }

    if (anyModifications)
    {
        emote.tooltip = Tooltip{
            QString("%1<br>%2 BetterTTV Emote<br>By: %3")
                .arg(emote.name.string)
                // when author is empty, it is a channel emote created by the broadcaster
                .arg(emote.author.string.isEmpty() ? "Channel" : "Shared")
                .arg(emote.author.string.isEmpty() ? channelDisplayName
                                                   : emote.author.string)};
    }

    return anyModifications;
}

}  // namespace

namespace chatterino {

std::weak_ptr<BttvEmoteProvider> BttvEmoteProvider::INSTANCE;

BttvEmoteProvider::BttvEmoteProvider()
    : BuiltinEmoteProvider(&getSettings()->enableBTTVGlobalEmotes,
                           GLOBAL_EMOTE_API_URL,
                           &getSettings()->enableBTTVChannelEmotes, u"BTTV"_s,
                           u"bttv"_s, BTTV_PRIORITY)
{
}

std::shared_ptr<BttvEmoteProvider> BttvEmoteProvider::instance()
{
    return INSTANCE.lock();
}

void BttvEmoteProvider::initialize()
{
    assert(INSTANCE.expired());
    INSTANCE =
        std::static_pointer_cast<BttvEmoteProvider>(this->shared_from_this());

    BuiltinEmoteProvider::initialize();
}

QString BttvEmoteProvider::emoteUrl(const Emote &emote) const
{
    return EMOTE_LINK_FORMAT.arg(emote.id.string);
}

EmotePtr BttvEmoteProvider::addEmote(
    TwitchChannel *channel, const BttvLiveUpdateEmoteUpdateAddMessage &message)
{
    auto result =
        createChannelEmote(channel->getLocalizedName(), message.jsonEmote);
    auto emote = this->createEmote(std::move(result.emote));
    if (this->addChannelEmote(channel->emotes(), emote))
    {
        return emote;
    }

    return nullptr;
}

std::optional<std::pair<EmotePtr, EmotePtr>> BttvEmoteProvider::updateEmote(
    TwitchChannel *channel, const BttvLiveUpdateEmoteUpdateAddMessage &message)
{
    return this->updateChannelEmote(
        channel->emotes(), message.emoteID, /*nameHint=*/QString{},
        [&](const auto &oldPtr) -> EmotePtr {
            Emote updated = *oldPtr;
            if (!updateBttvChannelEmote(updated, channel->getLocalizedName(),
                                        message.jsonEmote))
            {
                // The emote wasn't actually updated
                return nullptr;
            }
            return this->createEmote(std::move(updated));
        });
}

EmotePtr BttvEmoteProvider::removeEmote(
    TwitchChannel *channel, const BttvLiveUpdateEmoteRemoveMessage &message)
{
    return this->removeChannelEmote(channel->emotes(), message.emoteID,
                                    /*nameHint=*/QString{});
}

std::optional<EmoteMap> BttvEmoteProvider::parseChannelEmotes(
    TwitchChannel &twitch, const QJsonValue &json)
{
    QJsonObject jsonRoot = json.toObject();
    QString channelDisplayName = twitch.getLocalizedName();

    auto emotes = EmoteMap();

    auto innerParse = [&](const char *key) {
        auto jsonEmotes = jsonRoot.value(key).toArray();
        for (auto jsonEmote : jsonEmotes)
        {
            auto emote =
                createChannelEmote(channelDisplayName, jsonEmote.toObject());

            emotes[emote.name] = this->createEmote(std::move(emote.emote));
        }
    };

    innerParse("channelEmotes");
    innerParse("sharedEmotes");

    return {std::move(emotes)};
}

std::optional<EmoteMap> BttvEmoteProvider::parseGlobalEmotes(
    const QJsonValue &json)
{
    auto emotes = EmoteMap();

    for (auto jsonEmote : json.toArray())
    {
        auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
        auto name = EmoteName{jsonEmote.toObject().value("code").toString()};

        auto emote = Emote({
            .name = name,
            .images = ImageSet{Image::fromUrl(getEmoteLinkV3(id, "1x"), 1,
                                              EMOTE_BASE_SIZE),
                               Image::fromUrl(getEmoteLinkV3(id, "2x"), 0.5,
                                              EMOTE_BASE_SIZE * 2),
                               Image::fromUrl(getEmoteLinkV3(id, "3x"), 0.25,
                                              EMOTE_BASE_SIZE * 4)},
            .tooltip = Tooltip{name.string + "<br>Global BetterTTV Emote"},
            .providerID = PROVIDER_ID,
            .zeroWidth = ZERO_WIDTH_GLOBALS.contains(name.string),
            .id = id,
            .author = {},
            .baseName = {},
        });

        emotes[name] = this->createEmote(std::move(emote));
    }

    return {std::move(emotes)};
}

QString BttvEmoteProvider::channelEmotesUrl(const TwitchChannel &twitch) const
{
    return CHANNEL_EMOTE_API_URL % twitch.roomId();
}

}  // namespace chatterino
