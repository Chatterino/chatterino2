#include "providers/seventv/SeventvEmotes.hpp"

#include "SeventvEmoteCache.hpp"
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
    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no 7TV channel emotes.");
    const QString emoteLinkFormat("https://7tv.app/emotes/%1");

    // TODO(nerix): add links to documentation (7tv.io)
    const QString apiUrlUser("https://7tv.io/v3/users/twitch/%1");
    const QString apiUrlGlobalEmoteSet("https://7tv.io/v3/emote-sets/global");

    static SeventvEmoteCache emoteCache;
    static std::mutex emoteCacheMutex;

    bool isZeroWidthActive(const QJsonObject &addedEmote)
    {
        auto flags = SeventvActiveEmoteFlags(
            SeventvActiveEmoteFlag(addedEmote.value("flags").toInt()));
        return flags.has(SeventvActiveEmoteFlag::ZeroWidth);
    }

    bool isZeroWidthRecommended(const QJsonObject &emoteData)
    {
        auto flags = SeventvEmoteFlags(
            SeventvEmoteFlag(emoteData.value("flags").toInt()));
        return flags.has(SeventvEmoteFlag::ZeroWidth);
    }

    ImageSet makeImageSet(const QJsonObject &jsonEmote)
    {
        auto host = jsonEmote["host"].toObject();
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
                continue;
            }

            double width = file["width"].toDouble();
            double scale = 1.0;
            if (baseWidth > 0.0)
            {
                scale = baseWidth / width;
            }
            else
            {
                baseWidth = width;
            }

            auto image = Image::fromUrl(
                {QString("https:%1/%2").arg(baseUrl, file["name"].toString())},
                scale);

            sizes[nextSize] = image;
            nextSize++;
        }

        if (nextSize < sizes.size())
        {  // this should be really rare
            if (nextSize == 0)
            {
                qCWarning(chatterinoSeventv)
                    << "Got file list without any eligible files";
                return ImageSet{};
            }
            for (; nextSize < sizes.size(); nextSize++)
            {
                sizes[nextSize] = Image::getEmpty();
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
        if (cached)
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
     * that will be added to the cache.
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
                            Url{emoteLinkFormat.arg(id.string)}, zeroWidth});

        return emote;
    }

    /**
     * Creates a new aliased or global emote where the base
     * emote isn't cached. The emote's images may be cached
     * already (supplied by <code>imageSet</code>.
     */
    Emote createAliasedOrGlobalEmote(const EmoteId &id, bool isGlobal,
                                     const QJsonObject &addedEmote,
                                     const QJsonObject &emoteData,
                                     WeakImageSet *cachedImages)
    {
        auto name = EmoteName{addedEmote["name"].toString()};
        auto author = EmoteAuthor{
            emoteData["owner"].toObject()["display_name"].toString()};
        bool zeroWidth = isZeroWidthActive(addedEmote);
        bool aliasedName =
            addedEmote["name"].toString() != emoteData["name"].toString();
        auto tooltip = aliasedName
                           ? createAliasedTooltip(name.string,
                                                  emoteData["name"].toString(),
                                                  author.string, false)
                           : createTooltip(name.string, author.string, false);
        auto imageSet = lockOrCreateImageSet(emoteData, cachedImages);
        if (!imageSet.second)
        {
            emoteCache.putImageSet(id, imageSet.first);
        }

        auto emote = Emote({name, imageSet.first, tooltip,
                            Url{emoteLinkFormat.arg(id.string)}, zeroWidth});

        return emote;
    }

    /**
     * Creates an aliased or global emote where
     * the base emote is cached.
     */
    Emote forkExistingEmote(const QJsonObject &addedEmote,
                            const QJsonObject &emoteData,
                            const EmotePtr &baseEmote, bool isGlobal)
    {
        auto name = EmoteName{addedEmote["name"].toString()};
        auto author = emoteData["owner"].toObject()["display_name"].toString();
        bool isAliased =
            addedEmote["name"].toString() != baseEmote->name.string;
        auto tooltip = isAliased ? createAliasedTooltip(name.string,
                                                        baseEmote->name.string,
                                                        author, isGlobal)
                                 : createTooltip(name.string, author, isGlobal);
        bool zeroWidth = isZeroWidthActive(addedEmote);

        auto emote = Emote(
            {name, baseEmote->images, tooltip, baseEmote->homePage, zeroWidth});
        return emote;
    }

    bool checkEmoteVisibility(const QJsonObject &emoteData)
    {
        if (!emoteData.value("listed").toBool())
        {
            return getSettings()->showUnlistedEmotes;
        }
        auto flags = SeventvEmoteFlags(
            SeventvEmoteFlag(emoteData.value("flags").toInt()));
        return !flags.has(SeventvEmoteFlag::ContentTwitchDisallowed);
    }

    EmotePtr createEmote(const QJsonObject &addedEmote,
                         const QJsonObject &emoteData, bool isGlobal)
    {
        auto emoteId = EmoteId{addedEmote["id"].toString()};
        bool isAliased =
            addedEmote["name"].toString() != emoteData["name"].toString() ||
            isZeroWidthActive(addedEmote) != isZeroWidthRecommended(emoteData);
        bool isCacheable = !isAliased && !isGlobal;

        if (auto cached = emoteCache.getEmote(emoteId))
        {
            if (isCacheable)
            {
                return cached;
            }
            return std::make_shared<const Emote>(
                forkExistingEmote(addedEmote, emoteData, cached, isGlobal));
        }

        auto *cachedImages = emoteCache.getImageSet(emoteId);
        if (isCacheable)
        {
            // Cache the entire emote
            return emoteCache.putEmote(
                emoteId, createBaseEmote(emoteId, emoteData, cachedImages));
        }

        auto emote = std::make_shared<const Emote>(createAliasedOrGlobalEmote(
            emoteId, isGlobal, addedEmote, emoteData, cachedImages));
        if (cachedImages == nullptr)
        {
            // Only cache the images, not the entire emote
            emoteCache.putImageSet(emoteId, emote->images);
        }
        return emote;
    }

    EmoteMap parseEmotes(const QJsonArray &jsonEmotes, bool isGlobal)
    {
        auto emotes = EmoteMap();
        std::lock_guard<std::mutex> guard(emoteCacheMutex);

        for (auto jsonEmote_ : jsonEmotes)
        {
            auto addedEmote = jsonEmote_.toObject();
            auto emoteData = addedEmote["data"].toObject();

            if (!checkEmoteVisibility(emoteData))
            {
                continue;
            }
            auto emote = createEmote(addedEmote, emoteData, isGlobal);
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

    NetworkRequest(apiUrlGlobalEmoteSet, NetworkRequestType::Get)
        .timeout(30000)
        .onSuccess([this](NetworkResult result) -> Outcome {
            QJsonArray parsedEmotes = result.parseJson()["emotes"].toArray();

            auto emoteMap = parseEmotes(parsedEmotes, true);
            qCDebug(chatterinoSeventv)
                << "Loaded" << emoteMap.size() << "7TV Global Emotes";
            this->global_.set(std::make_shared<EmoteMap>(std::move(emoteMap)));

            return Success;
        })
        .onError([](NetworkResult result) {
            qCWarning(chatterinoSeventv)
                << "Couldn't load 7TV global emotes" << result.getData();
        })
        .execute();
}

void SeventvEmotes::loadChannelEmotes(std::weak_ptr<Channel> channel,
                                      const QString &channelId,
                                      std::function<void(EmoteMap &&)> callback,
                                      bool manualRefresh)
{
    qCDebug(chatterinoSeventv)
        << "Reloading 7TV Channel Emotes" << channelId << manualRefresh;

    NetworkRequest(apiUrlUser.arg(channelId), NetworkRequestType::Get)
        .timeout(20000)
        .onSuccess([callback = std::move(callback), channel, channelId,
                    manualRefresh](NetworkResult result) -> Outcome {
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
        .onError([channelId, channel, manualRefresh](NetworkResult result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 400)
            {
                qCWarning(chatterinoSeventv)
                    << "Error occurred fetching 7TV emotes: "
                    << result.parseJson();
                if (manualRefresh)
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
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