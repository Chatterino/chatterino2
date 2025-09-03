#include "providers/seventv/SeventvEmoteProvider.hpp"

#include "Application.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/SeventvAPI.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "util/Functional.hpp"
#include "util/Helpers.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QStringView>
#include <QThread>

/**
 * # References
 *
 * - EmoteSet: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L8-L18
 * - ActiveEmote: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L20-L27
 * - EmotePartial (emoteData): https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote.model.go#L24-L34
 * - ImageHost: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/model.go#L36-L39
 * - ImageFile: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/model.go#L41-L48
 */
namespace {

using namespace chatterino;
using namespace seventv::eventapi;
using namespace functional;
using namespace Qt::Literals;

// These declarations won't throw an exception.
const QString CHANNEL_HAS_NO_EMOTES("This channel has no 7TV channel emotes.");
const QString EMOTE_LINK_FORMAT("https://7tv.app/emotes/%1");

const QString PROVIDER_ID = u"seventv"_s;
const QString PROVIDER_NAME = u"7TV"_s;

// https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L29-L36
enum class SeventvActiveEmoteFlag : uint32_t {
    None = 0LL,

    // Emote is zero-width
    ZeroWidth = (1LL << 0),

    // Overrides Twitch Global emotes with the same name
    OverrideTwitchGlobal = (1 << 16),
    // Overrides Twitch Subscriber emotes with the same name
    OverrideTwitchSubscriber = (1 << 17),
    // Overrides BetterTTV emotes with the same name
    OverrideBetterTTV = (1 << 18),
};

// https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote.model.go#L57-L70
enum class SeventvEmoteFlag : uint32_t {
    None = 0LL,
    // The emote is private and can only be accessed by its owner, editors and moderators
    Private = 1 << 0,
    // The emote was verified to be an original creation by the uploader
    Authentic = (1LL << 1),
    // The emote is recommended to be enabled as Zero-Width
    ZeroWidth = (1LL << 8),

    // Content Flags

    // Sexually Suggesive
    ContentSexual = (1LL << 16),
    // Rapid flashing
    ContentEpilepsy = (1LL << 17),
    // Edgy or distasteful, may be offensive to some users
    ContentEdgy = (1 << 18),
    // Not allowed specifically on the Twitch platform
    ContentTwitchDisallowed = (1LL << 24),
};

using SeventvActiveEmoteFlags = FlagsEnum<SeventvActiveEmoteFlag>;
using SeventvEmoteFlags = FlagsEnum<SeventvEmoteFlag>;

enum class SeventvEmoteSetKind : uint8_t {
    Global,
    Personal,
    Channel,
};

enum class SeventvEmoteSetFlag : uint8_t {
    Immutable = (1 << 0),
    Privileged = (1 << 1),
    Personal = (1 << 2),
    Commercial = (1 << 3),
};
using SeventvEmoteSetFlags = FlagsEnum<SeventvEmoteSetFlag>;

struct CreateEmoteResult {
    Emote emote;
    EmoteId id;
    EmoteName name;
    bool hasImages{};
};

/**
  * This decides whether an emote should be displayed
  * as zero-width
  */
bool isZeroWidthActive(const QJsonObject &activeEmote)
{
    auto flags = SeventvActiveEmoteFlags(
        SeventvActiveEmoteFlag(activeEmote.value("flags").toInt()));
    return flags.has(SeventvActiveEmoteFlag::ZeroWidth);
}

Tooltip createTooltip(const QString &name, const QString &author, bool isGlobal)
{
    return Tooltip{QString("%1<br>%2 7TV Emote<br>By: %3")
                       .arg(name.toHtmlEscaped(),
                            isGlobal ? "Global" : "Channel",
                            author.isEmpty() ? "&lt;deleted&gt;"
                                             : author.toHtmlEscaped())};
}

Tooltip createAliasedTooltip(const QString &name, const QString &baseName,
                             const QString &author, bool isGlobal)
{
    return Tooltip{QString("%1<br>Alias of %2<br>%3 7TV Emote<br>By: %4")
                       .arg(name.toHtmlEscaped(), baseName.toHtmlEscaped(),
                            isGlobal ? "Global" : "Channel",
                            author.isEmpty() ? "&lt;deleted&gt;"
                                             : author.toHtmlEscaped())};
}

CreateEmoteResult createSeventvEmote(const QJsonObject &activeEmote,
                                     const QJsonObject &emoteData,
                                     bool isGlobal)
{
    auto emoteId = EmoteId{activeEmote["id"].toString()};
    auto emoteName = EmoteName{activeEmote["name"].toString()};
    auto author =
        EmoteAuthor{emoteData["owner"].toObject()["display_name"].toString()};
    auto baseEmoteName = EmoteName{emoteData["name"].toString()};
    bool zeroWidth = isZeroWidthActive(activeEmote);
    bool aliasedName = emoteName != baseEmoteName;
    auto tooltip =
        aliasedName
            ? createAliasedTooltip(emoteName.string, baseEmoteName.string,
                                   author.string, isGlobal)
            : createTooltip(emoteName.string, author.string, isGlobal);
    auto imageSet = SeventvEmoteProvider::createImageSet(emoteData, false);

    auto emote = Emote({
        emoteName,
        imageSet,
        tooltip,
        PROVIDER_ID,
        zeroWidth,
        emoteId,
        author,
        makeConditionedOptional(aliasedName, baseEmoteName),
    });

    return {emote, emoteId, emoteName, !emote.images.getImage1()->isEmpty()};
}

bool checkEmoteVisibility(const QJsonObject &emoteData)
{
    if (!emoteData["listed"].toBool() &&
        !getSettings()->showUnlistedSevenTVEmotes)
    {
        return false;
    }
    auto flags =
        SeventvEmoteFlags(SeventvEmoteFlag(emoteData["flags"].toInt()));
    return !flags.has(SeventvEmoteFlag::ContentTwitchDisallowed);
}

EmotePtr createUpdatedEmote(const EmotePtr &oldEmote,
                            const EmoteUpdateDispatch &dispatch)
{
    bool toNonAliased = oldEmote->baseName.has_value() &&
                        dispatch.emoteName == oldEmote->baseName->string;

    auto baseName = oldEmote->baseName.value_or(oldEmote->name);
    auto emote = std::make_shared<const Emote>(Emote(
        {EmoteName{dispatch.emoteName}, oldEmote->images,
         toNonAliased
             ? createTooltip(dispatch.emoteName, oldEmote->author.string, false)
             : createAliasedTooltip(dispatch.emoteName, baseName.string,
                                    oldEmote->author.string, false),
         oldEmote->providerID, oldEmote->zeroWidth, oldEmote->id,
         oldEmote->author, makeConditionedOptional(!toNonAliased, baseName)}));
    return emote;
}

}  // namespace

namespace chatterino {

using namespace seventv::eventapi;

namespace seventv::detail {

std::optional<Emote> parseEmote(const QJsonObject &activeEmote, bool isGlobal)
{
    auto emoteData = activeEmote["data"].toObject();

    if (emoteData.empty() || !checkEmoteVisibility(emoteData))
    {
        return std::nullopt;
    }

    auto result = createSeventvEmote(activeEmote, emoteData, isGlobal);
    if (!result.hasImages)
    {
        // this shouldn't happen but if it does, it will crash,
        // so we don't add the emote
        qCDebug(chatterinoSeventv) << "Emote without images:" << activeEmote;
        return std::nullopt;
    }

    return {std::move(result.emote)};
}

}  // namespace seventv::detail

std::weak_ptr<SeventvEmoteProvider> SeventvEmoteProvider::INSTANCE;

SeventvEmoteProvider::SeventvEmoteProvider()
    : BuiltinEmoteProvider(&getSettings()->enableSevenTVGlobalEmotes,
                           SeventvAPI::API_URL_EMOTE_SET.arg("global"),
                           &getSettings()->enableSevenTVChannelEmotes,
                           PROVIDER_NAME, PROVIDER_ID, SEVENTV_PRIORITY)
{
}

std::shared_ptr<SeventvEmoteProvider> SeventvEmoteProvider::instance()
{
    return INSTANCE.lock();
}

std::shared_ptr<SeventvEmoteProvider> SeventvEmoteProvider::shared()
{
    return std::static_pointer_cast<SeventvEmoteProvider>(
        this->shared_from_this());
}

void SeventvEmoteProvider::initialize()
{
    assert(INSTANCE.expired());
    INSTANCE = this->shared();
    BuiltinEmoteProvider::initialize();
}

QString SeventvEmoteProvider::emoteUrl(const Emote &emote) const
{
    return EMOTE_LINK_FORMAT.arg(emote.id.string);
}

std::optional<EmoteMap> SeventvEmoteProvider::parseChannelEmotes(
    TwitchChannel &twitch, const QJsonValue &json)
{
    const auto emoteSet = json["emote_set"].toObject();
    const auto parsedEmotes = emoteSet["emotes"].toArray();

    auto emoteMap = this->parseEmotes(parsedEmotes, false);

    qCDebug(chatterinoSeventv) << "Loaded" << emoteMap.size()
                               << "7TV Channel Emotes for" << twitch.roomId();

    auto user = json["user"].toObject();

    size_t connectionIdx = 0;
    for (const auto &conn : user["connections"].toArray())
    {
        if (conn.toObject()["platform"].toString() == "TWITCH")
        {
            break;
        }
        connectionIdx++;
    }
    twitch.updateSeventvData(user["id"].toString(), emoteSet["id"].toString(),
                             connectionIdx);

    return {std::move(emoteMap)};
}

std::optional<EmoteMap> SeventvEmoteProvider::parseGlobalEmotes(
    const QJsonValue &json)
{
    QJsonArray emotes = json["emotes"].toArray();

    return this->parseEmotes(emotes, true);
}

QString SeventvEmoteProvider::channelEmotesUrl(
    const TwitchChannel &twitch) const
{
    return SeventvAPI::API_URL_USER.arg(twitch.roomId());
}

EmoteMap SeventvEmoteProvider::parseEmotes(const QJsonArray &emoteSetEmotes,
                                           bool isGlobal)
{
    auto emotes = EmoteMap();

    for (const auto &activeEmoteJson : emoteSetEmotes)
    {
        auto emote =
            seventv::detail::parseEmote(activeEmoteJson.toObject(), isGlobal);
        if (!emote)
        {
            continue;
        }
        auto name = emote->name;
        auto ptr = this->createEmote(*std::move(emote));
        emotes[name] = ptr;
    }

    return emotes;
}

EmotePtr SeventvEmoteProvider::addEmote(
    TwitchChannel *channel, const seventv::eventapi::EmoteAddDispatch &message)
{
    // Check for visibility first, so we don't copy the map.
    auto emoteData = message.emoteJson["data"].toObject();
    if (emoteData.empty() || !checkEmoteVisibility(emoteData))
    {
        return nullptr;
    }

    auto result = createSeventvEmote(message.emoteJson, emoteData, false);
    if (!result.hasImages)
    {
        // Incoming emote didn't contain any images, abort
        qCDebug(chatterinoSeventv)
            << "Emote without images:" << message.emoteJson;
        return nullptr;
    }

    auto emote = this->createEmote(std::move(result.emote));
    if (this->addChannelEmote(channel->emotes(), emote))
    {
        return emote;
    }
    return nullptr;
}

std::optional<std::pair<EmotePtr, EmotePtr>> SeventvEmoteProvider::updateEmote(
    TwitchChannel *channel,
    const seventv::eventapi::EmoteUpdateDispatch &message)
{
    return this->updateChannelEmote(
        channel->emotes(), message.emoteID, message.oldEmoteName,
        [&](const auto &oldPtr) {
            return createUpdatedEmote(oldPtr, message);
        });
}

EmotePtr SeventvEmoteProvider::removeEmote(
    TwitchChannel *channel,
    const seventv::eventapi::EmoteRemoveDispatch &message)
{
    return this->removeChannelEmote(channel->emotes(), message.emoteID,
                                    message.emoteName);
}

void SeventvEmoteProvider::applyEmoteSet(
    TwitchChannel *channel, const QString &emoteSetID,
    const std::function<void(ExpectedStr<EmoteSetData>)> &onDone)
{
    qCDebug(chatterinoSeventv) << "Loading 7TV Emote Set" << emoteSetID;

    auto chanPtr =
        std::static_pointer_cast<TwitchChannel>(channel->shared_from_this());

    getApp()->getSeventvAPI()->getEmoteSet(
        emoteSetID,
        weakGuarded(
            [emoteSetID, onDone](const auto &self, const auto &channel,
                                 const auto &json) {
                assert(!isAppAboutToQuit());
                auto *item = channel->emotes().itemByProvider(self);
                if (!item)
                {
                    return;
                }

                auto parsedEmotes = json["emotes"].toArray();

                auto emoteMap = self->parseEmotes(parsedEmotes, false);
                item->emotes =
                    std::make_shared<const EmoteMap>(std::move(emoteMap));

                qCDebug(chatterinoSeventv) << "Loaded" << emoteMap.size()
                                           << "7TV Emotes from" << emoteSetID;
                onDone(EmoteSetData{
                    .name = json["name"].toString(),
                });
            },
            this->shared(), chanPtr),
        [emoteSetID, onDone](const auto &result) {
            onDone(makeUnexpected(result.formatError()));
        });
}

ImageSet SeventvEmoteProvider::createImageSet(const QJsonObject &emoteData,
                                              bool useStatic)
{
    auto host = emoteData["host"].toObject();
    // "//cdn.7tv[...]"
    auto baseUrl = host["url"].toString();
    auto files = host["files"].toArray();

    std::array<ImagePtr, 4> sizes;
    double baseWidth = 0.0;
    size_t nextSize = 0;

    for (auto fileItem : files)
    {
        if (nextSize >= sizes.size())
        {
            break;
        }

        auto file = fileItem.toObject();
        if (file["format"].toString() != "WEBP")
        {
            continue;  // We only use webp
        }

        double width = file["width"].toDouble();
        double scale = 1.0;  // in relation to first image
        if (baseWidth > 0.0)
        {
            scale = baseWidth / width;
        }
        else
        {
            // => this is the first image
            baseWidth = width;
        }

        auto name = [&] {
            if (useStatic)
            {
                auto staticName = file["static_name"].toString();
                if (!staticName.isEmpty())
                {
                    return staticName;
                }
            }
            return file["name"].toString();
        }();

        auto image =
            Image::fromUrl({QString("https:%1/%2").arg(baseUrl, name)}, scale,
                           {static_cast<int>(width), file["height"].toInt(16)});

        sizes.at(nextSize) = image;
        nextSize++;
    }

    if (nextSize < sizes.size())
    {
        // this should be really rare
        // this means we didn't get all sizes of an emote
        if (nextSize == 0)
        {
            qCDebug(chatterinoSeventv)
                << "Got file list without any eligible files";
            // When this emote is typed, chatterino will crash.
            return ImageSet{};
        }
        for (; nextSize < sizes.size(); nextSize++)
        {
            sizes.at(nextSize) = Image::getEmpty();
        }
    }

    // Typically, 7TV provides four versions (1x, 2x, 3x, and 4x). The 3x
    // version has a scale factor of 1/3, which is a size other providers don't
    // provide - they only provide the 4x version (0.25). To be in line with
    // other providers, we prefer the 4x version but fall back to the 3x one if
    // it doesn't exist.
    auto largest = std::move(sizes[3]);
    if (!largest || largest->isEmpty())
    {
        largest = std::move(sizes[2]);
    }

    return ImageSet{sizes[0], sizes[1], largest};
}

}  // namespace chatterino
