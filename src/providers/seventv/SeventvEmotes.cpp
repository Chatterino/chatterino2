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
/**
 * # Caching
 *
 *
 *                                   (activeEmote, emoteData)
 *                                             |
 *                                          is aliased?
 *                 +------------------------y--+--n-------------------------------+
 *                 |                                                              |
 *            non-aliased                                                    emote cached?
 *            emote cached?                                    +---------------y--+--n--------------+
 *       +-----y--+--n-------------+                           |                                    |
 *       |                         |                      use cached emote                      images cached?
 *  fork emote                 images cached?                                          +---------y--+--n-----------+
 *  (use images         +------ y--+--n-------+                                        |                           |
 *  and homepage)       |                     |                                    use images,                create emote,
 *                  use cached            create emote,                           create emote,                cache emote
 *                    images,             cache images,                         remove cache entry,
 *                  don't cache           don't cache                              cache emote
 *                  emote itself          emote itself
 *
 * # References
 *
 * - EmoteSet: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L8-L18
 * - ActiveEmote: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L20-L27
 * - EmotePartial (emoteData): https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote.model.go#L24-L34
 * - ImageHost: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/model.go#L36-L39
 * - ImageFile: https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/model.go#L41-L48
 */
namespace {
    // These declarations won't throw an exception.
    // NOLINTBEGIN(cert-err58-cpp)
    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no 7TV channel emotes.");
    const QString EMOTE_LINK_FORMAT("https://7tv.app/emotes/%1");

    // TODO(nerix): add links to documentation (7tv.io)
    const QString API_URL_USER("https://7tv.io/v3/users/twitch/%1");
    const QString API_URL_GLOBAL_EMOTE_SET(
        "https://7tv.io/v3/emote-sets/global");

    // We can't declare these as const, but we make sure to only
    // access `EMOTE_CACHE` while locking `EMOTE_CACHE_MUTEX`.
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    SeventvEmoteCache EMOTE_CACHE;
    std::mutex EMOTE_CACHE_MUTEX;
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    // NOLINTEND(cert-err58-cpp)

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

    /**
     * This is only an indicator if an emote should be added
     * as zero-width or not. The user can still overwrite this.
     */
    bool isZeroWidthRecommended(const QJsonObject &emoteData)
    {
        auto flags = SeventvEmoteFlags(
            SeventvEmoteFlag(emoteData.value("flags").toInt()));
        return flags.has(SeventvEmoteFlag::ZeroWidth);
    }

    ImageSet makeImageSet(const QJsonObject &emoteData)
    {
        auto host = emoteData["host"].toObject();
        // "//cdn.7tv[...]"
        auto baseUrl = host["url"].toString();
        auto files = host["files"].toArray();

        std::array<ImagePtr, 4> sizes;
        double baseWidth = 0.0;
        int nextSize = 0;

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

            auto image = Image::fromUrl(
                {QString("https:%1/%2").arg(baseUrl, file["name"].toString())},
                scale);

            sizes.at(nextSize) = image;
            nextSize++;
        }

        if (nextSize < sizes.size())
        {
            // this should be really rare
            // this means we didn't get all sizes of an emote
            if (nextSize == 0)
            {
                qCWarning(chatterinoSeventv)
                    << "Got file list without any eligible files";
                // When this emote is typed, chatterino will segfault.
                // TODO: provide fallback?
                return ImageSet{};
            }
            for (; nextSize < sizes.size(); nextSize++)
            {
                sizes.at(nextSize) = Image::getEmpty();
            }
        }

        return ImageSet{sizes[0], sizes[1], sizes[2], sizes[3]};
    }

    Tooltip createTooltip(const QString &name, const QString &author,
                          bool isGlobal)
    {
        return Tooltip{QString("%1<br>%2 7TV Emote<br>By: %3")
                           .arg(name, isGlobal ? "Global" : "Channel",
                                author.isEmpty() ? "<deleted>" : author)};
    }

    Tooltip createAliasedTooltip(const QString &name, const QString &baseName,
                                 const QString &author, bool isGlobal)
    {
        return Tooltip{QString("%1<br>Alias to %2<br>%3 7TV Emote<br>By: %4")
                           .arg(name, baseName, isGlobal ? "Global" : "Channel",
                                author.isEmpty() ? "<deleted>" : author)};
    }

    /**
     * @return (imageSet, fromCache)
     */
    std::pair<ImageSet, bool> lockOrCreateImageSet(const QJsonObject &emoteData,
                                                   WeakImageSet *cached)
    {
        if (cached != nullptr)
        {
            if (auto locked = cached->lock())
            {
                return std::make_pair(locked.get(), true);
            }
        }
        return std::make_pair(makeImageSet(emoteData), false);
    }

    /**
     * Creates a "regular" (i.e. not aliased or global) emote
     * that may be added to the cache.
     */
    Emote createBaseEmote(const EmoteId &id, const QJsonObject &emoteData,
                          WeakImageSet *cached)
    {
        auto name = EmoteName{emoteData.value("name").toString()};
        auto author = EmoteAuthor{emoteData.value("owner")
                                      .toObject()
                                      .value("display_name")
                                      .toString()};
        bool zeroWidth = isZeroWidthRecommended(emoteData);
        // This isn't cached since the entire emote will be cached
        auto imageSet = lockOrCreateImageSet(emoteData, cached).first;

        auto emote = Emote({name, imageSet,
                            createTooltip(name.string, author.string, false),
                            Url{EMOTE_LINK_FORMAT.arg(id.string)}, zeroWidth});

        return emote;
    }

    /**
     * Creates a new aliased or global emote where the base
     * emote isn't cached. The emote's images may be cached
     * already.
     *
     * @return (emote, imagesFromCache)
     */
    std::pair<Emote, bool> createAliasedOrGlobalEmote(
        const EmoteId &id, bool isGlobal, const QJsonObject &activeEmote,
        const QJsonObject &emoteData, WeakImageSet *cachedImages)
    {
        auto name = EmoteName{activeEmote["name"].toString()};
        auto author = EmoteAuthor{
            emoteData["owner"].toObject()["display_name"].toString()};
        auto baseEmoteName = emoteData["name"].toString();
        bool zeroWidth = isZeroWidthActive(activeEmote);
        bool aliasedName = name.string != baseEmoteName;
        auto tooltip =
            aliasedName ? createAliasedTooltip(name.string, baseEmoteName,
                                               author.string, isGlobal)
                        : createTooltip(name.string, author.string, isGlobal);
        auto imageSet = lockOrCreateImageSet(emoteData, cachedImages);

        auto emote = Emote({name, imageSet.first, tooltip,
                            Url{EMOTE_LINK_FORMAT.arg(id.string)}, zeroWidth});

        return std::make_pair(emote, imageSet.second);
    }

    /**
     * Creates an aliased or global emote where
     * the base emote is cached.
     */
    Emote forkExistingEmote(const QJsonObject &activeEmote,
                            const QJsonObject &emoteData,
                            const EmotePtr &baseEmote, bool isGlobal)
    {
        auto name = EmoteName{activeEmote["name"].toString()};
        auto author = emoteData["owner"].toObject()["display_name"].toString();
        bool isAliased =
            activeEmote["name"].toString() != baseEmote->name.string;
        auto tooltip = isAliased ? createAliasedTooltip(name.string,
                                                        baseEmote->name.string,
                                                        author, isGlobal)
                                 : createTooltip(name.string, author, isGlobal);
        bool zeroWidth = isZeroWidthActive(activeEmote);

        auto emote = Emote(
            {name, baseEmote->images, tooltip, baseEmote->homePage, zeroWidth});
        return emote;
    }

    bool checkEmoteVisibility(const QJsonObject &emoteData)
    {
        if (!emoteData["listed"].toBool() && !getSettings()->showUnlistedEmotes)
        {
            return false;
        }
        auto flags =
            SeventvEmoteFlags(SeventvEmoteFlag(emoteData["flags"].toInt()));
        return !flags.has(SeventvEmoteFlag::ContentTwitchDisallowed);
    }

    EmotePtr createEmote(const QJsonObject &activeEmote,
                         const QJsonObject &emoteData, bool isGlobal)
    {
        auto emoteId = EmoteId{activeEmote["id"].toString()};
        bool isAliased =
            activeEmote["name"].toString() != emoteData["name"].toString() ||
            isZeroWidthActive(activeEmote) != isZeroWidthRecommended(emoteData);
        bool isCacheable = !isAliased && !isGlobal;

        if (auto cached = EMOTE_CACHE.getEmote(emoteId))
        {
            if (isCacheable)
            {
                return cached;
            }
            return std::make_shared<const Emote>(
                forkExistingEmote(activeEmote, emoteData, cached, isGlobal));
        }

        auto *cachedImages = EMOTE_CACHE.getImageSet(emoteId);
        if (isCacheable)
        {
            // Cache the entire emote
            return EMOTE_CACHE.putEmote(
                emoteId, createBaseEmote(emoteId, emoteData, cachedImages));
        }

        auto emoteResult = createAliasedOrGlobalEmote(
            emoteId, isGlobal, activeEmote, emoteData, cachedImages);

        auto emote = std::make_shared<const Emote>(emoteResult.first);
        if (!emoteResult.second)
        {
            // Only cache the images, not the entire emote
            EMOTE_CACHE.putImageSet(emoteId, emote->images);
        }
        return emote;
    }

    EmoteMap parseEmotes(const QJsonArray &emoteSetEmotes, bool isGlobal)
    {
        auto emotes = EmoteMap();
        std::lock_guard<std::mutex> guard(EMOTE_CACHE_MUTEX);

        for (auto activeEmoteJson : emoteSetEmotes)
        {
            auto activeEmote = activeEmoteJson.toObject();
            auto emoteData = activeEmote["data"].toObject();

            if (!checkEmoteVisibility(emoteData))
            {
                continue;
            }
            auto emote = createEmote(activeEmote, emoteData, isGlobal);
            emotes[emote->name] = emote;
        }

        return emotes;
    }

}  // namespace

SeventvEmotes::SeventvEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> SeventvEmotes::globalEmotes() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> SeventvEmotes::globalEmote(
    const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
    {
        return boost::none;
    }
    return it->second;
}

void SeventvEmotes::loadGlobalEmotes()
{
    if (!Settings::instance().enableSevenTVGlobalEmotes)
    {
        this->global_.set(EMPTY_EMOTE_MAP);
        return;
    }

    qCDebug(chatterinoSeventv) << "Loading 7TV Global Emotes";

    NetworkRequest(API_URL_GLOBAL_EMOTE_SET, NetworkRequestType::Get)
        .timeout(30000)
        .onSuccess([this](const NetworkResult &result) -> Outcome {
            QJsonArray parsedEmotes = result.parseJson()["emotes"].toArray();

            auto emoteMap = parseEmotes(parsedEmotes, true);
            qCDebug(chatterinoSeventv)
                << "Loaded" << emoteMap.size() << "7TV Global Emotes";
            this->global_.set(std::make_shared<EmoteMap>(std::move(emoteMap)));

            return Success;
        })
        .onError([](const NetworkResult &result) {
            qCWarning(chatterinoSeventv)
                << "Couldn't load 7TV global emotes" << result.getData();
        })
        .execute();
}

void SeventvEmotes::loadChannelEmotes(const std::weak_ptr<Channel> &channel,
                                      const QString &channelId,
                                      std::function<void(EmoteMap &&)> callback,
                                      bool manualRefresh)
{
    qCDebug(chatterinoSeventv)
        << "Reloading 7TV Channel Emotes" << channelId << manualRefresh;

    NetworkRequest(API_URL_USER.arg(channelId), NetworkRequestType::Get)
        .timeout(20000)
        .onSuccess([callback = std::move(callback), channel, channelId,
                    manualRefresh](const NetworkResult &result) -> Outcome {
            auto json = result.parseJson();
            auto emoteSet = json["emote_set"].toObject();
            auto parsedEmotes = emoteSet["emotes"].toArray();

            auto emoteMap = parseEmotes(parsedEmotes, false);
            bool hasEmotes = !emoteMap.empty();

            qCDebug(chatterinoSeventv)
                << "Loaded" << emoteMap.size() << "7TV Channel Emotes for"
                << channelId << "manual refresh:" << manualRefresh;

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
        .onError(
            [channelId, channel, manualRefresh](const NetworkResult &result) {
                auto shared = channel.lock();
                if (!shared)
                {
                    return;
                }
                if (result.status() == 400)
                {
                    qCWarning(chatterinoSeventv)
                        << "Error occurred fetching 7TV emotes: "
                        << result.parseJson();
                    if (manualRefresh)
                    {
                        shared->addMessage(
                            makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                    }
                }
                else if (result.status() == NetworkResult::timedoutStatus)
                {
                    // TODO: Auto retry in case of a timeout, with a delay
                    qCWarning(chatterinoSeventv)
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