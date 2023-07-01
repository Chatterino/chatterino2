#include "providers/bttv/BttvEmotes.hpp"

#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QJsonArray>
#include <QThread>

namespace chatterino {
namespace {

    const QString CHANNEL_HAS_NO_EMOTES(
        "This channel has no BetterTTV channel emotes.");

    QString emoteLinkFormat("https://betterttv.com/emotes/%1");

    struct CreateEmoteResult {
        EmoteId id;
        EmoteName name;
        Emote emote;
    };

    Url getEmoteLink(QString urlTemplate, const EmoteId &id,
                     const QString &emoteScale)
    {
        urlTemplate.detach();

        return {urlTemplate.replace("{{id}}", id.string)
                    .replace("{{image}}", emoteScale)};
    }

    Url getEmoteLinkV3(const EmoteId &id, const QString &emoteScale)
    {
        static const QString urlTemplate(
            "https://cdn.betterttv.net/emote/%1/%2");

        return {urlTemplate.arg(id.string, emoteScale)};
    }
    EmotePtr cachedOrMake(Emote &&emote, const EmoteId &id)
    {
        static std::unordered_map<EmoteId, std::weak_ptr<const Emote>> cache;
        static std::mutex mutex;

        return cachedOrMakeEmotePtr(std::move(emote), cache, mutex, id);
    }
    std::pair<Outcome, EmoteMap> parseGlobalEmotes(
        const QJsonArray &jsonEmotes, const EmoteMap &currentEmotes)
    {
        auto emotes = EmoteMap();

        for (auto jsonEmote : jsonEmotes)
        {
            auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
            auto name =
                EmoteName{jsonEmote.toObject().value("code").toString()};

            auto emote = Emote({
                name,
                ImageSet{Image::fromUrl(getEmoteLinkV3(id, "1x"), 1),
                         Image::fromUrl(getEmoteLinkV3(id, "2x"), 0.5),
                         Image::fromUrl(getEmoteLinkV3(id, "3x"), 0.25)},
                Tooltip{name.string + "<br>Global BetterTTV Emote"},
                Url{emoteLinkFormat.arg(id.string)},
            });

            emotes[name] =
                cachedOrMakeEmotePtr(std::move(emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }

    CreateEmoteResult createChannelEmote(const QString &channelDisplayName,
                                         const QJsonObject &jsonEmote)
    {
        auto id = EmoteId{jsonEmote.value("id").toString()};
        auto name = EmoteName{jsonEmote.value("code").toString()};
        auto author = EmoteAuthor{
            jsonEmote.value("user").toObject().value("displayName").toString()};

        auto emote = Emote({
            name,
            ImageSet{
                Image::fromUrl(getEmoteLinkV3(id, "1x"), 1),
                Image::fromUrl(getEmoteLinkV3(id, "2x"), 0.5),
                Image::fromUrl(getEmoteLinkV3(id, "3x"), 0.25),
            },
            Tooltip{
                QString("%1<br>%2 BetterTTV Emote<br>By: %3")
                    .arg(name.string)
                    // when author is empty, it is a channel emote created by the broadcaster
                    .arg(author.string.isEmpty() ? "Channel" : "Shared")
                    .arg(author.string.isEmpty() ? channelDisplayName
                                                 : author.string)},
            Url{emoteLinkFormat.arg(id.string)},
            false,
            id,
        });

        return {id, name, emote};
    }

    bool updateChannelEmote(Emote &emote, const QString &channelDisplayName,
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
            emote.author = EmoteAuthor{jsonEmote.value("user")
                                           .toObject()
                                           .value("displayName")
                                           .toString()};
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

    std::pair<Outcome, EmoteMap> parseChannelEmotes(
        const QJsonObject &jsonRoot, const QString &channelDisplayName)
    {
        auto emotes = EmoteMap();

        auto innerParse = [&jsonRoot, &emotes,
                           &channelDisplayName](const char *key) {
            auto jsonEmotes = jsonRoot.value(key).toArray();
            for (auto jsonEmote_ : jsonEmotes)
            {
                auto emote = createChannelEmote(channelDisplayName,
                                                jsonEmote_.toObject());

                emotes[emote.name] =
                    cachedOrMake(std::move(emote.emote), emote.id);
            }
        };

        innerParse("channelEmotes");
        innerParse("sharedEmotes");

        return {Success, std::move(emotes)};
    }
}  // namespace

//
// BttvEmotes
//
BttvEmotes::BttvEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> BttvEmotes::emotes() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> BttvEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

void BttvEmotes::loadEmotes()
{
    if (!Settings::instance().enableBTTVGlobalEmotes)
    {
        this->setEmotes(EMPTY_EMOTE_MAP);
        return;
    }

    NetworkRequest(QString(globalEmoteApiUrl))
        .timeout(30000)
        .onSuccess([this](auto result) -> Outcome {
            auto emotes = this->global_.get();
            auto pair = parseGlobalEmotes(result.parseJsonArray(), *emotes);
            if (pair.first)
                this->setEmotes(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

void BttvEmotes::setEmotes(std::shared_ptr<const EmoteMap> emotes)
{
    this->global_.set(std::move(emotes));
}

void BttvEmotes::loadChannel(std::weak_ptr<Channel> channel,
                             const QString &channelId,
                             const QString &channelDisplayName,
                             std::function<void(EmoteMap &&)> callback,
                             bool manualRefresh)
{
    NetworkRequest(QString(bttvChannelEmoteApiUrl) + channelId)
        .timeout(20000)
        .onSuccess([callback = std::move(callback), channel, channelDisplayName,
                    manualRefresh](auto result) -> Outcome {
            auto pair =
                parseChannelEmotes(result.parseJson(), channelDisplayName);
            bool hasEmotes = false;
            if (pair.first)
            {
                hasEmotes = !pair.second.empty();
                callback(std::move(pair.second));
            }
            if (auto shared = channel.lock(); manualRefresh)
            {
                if (hasEmotes)
                {
                    shared->addMessage(makeSystemMessage(
                        "BetterTTV channel emotes reloaded."));
                }
                else
                {
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
                }
            }
            return pair.first;
        })
        .onError([channelId, channel, manualRefresh](auto result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 404)
            {
                // User does not have any BTTV emotes
                if (manualRefresh)
                    shared->addMessage(
                        makeSystemMessage(CHANNEL_HAS_NO_EMOTES));
            }
            else
            {
                // TODO: Auto retry in case of a timeout, with a delay
                auto errorString = result.formatError();
                qCWarning(chatterinoBttv)
                    << "Error fetching BTTV emotes for channel" << channelId
                    << ", error" << errorString;
                shared->addMessage(makeSystemMessage(
                    QStringLiteral("Failed to fetch BetterTTV channel "
                                   "emotes. (Error: %1)")
                        .arg(errorString)));
            }
        })
        .execute();
}

EmotePtr BttvEmotes::addEmote(
    const QString &channelDisplayName,
    Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
    const BttvLiveUpdateEmoteUpdateAddMessage &message)
{
    // This copies the map.
    EmoteMap updatedMap = *channelEmoteMap.get();
    auto result = createChannelEmote(channelDisplayName, message.jsonEmote);

    auto emote = std::make_shared<const Emote>(std::move(result.emote));
    updatedMap[result.name] = emote;
    channelEmoteMap.set(std::make_shared<EmoteMap>(std::move(updatedMap)));

    return emote;
}

boost::optional<std::pair<EmotePtr, EmotePtr>> BttvEmotes::updateEmote(
    const QString &channelDisplayName,
    Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
    const BttvLiveUpdateEmoteUpdateAddMessage &message)
{
    // This copies the map.
    EmoteMap updatedMap = *channelEmoteMap.get();

    // Step 1: remove the existing emote
    auto it = updatedMap.findEmote(QString(), message.emoteID);
    if (it == updatedMap.end())
    {
        // We already copied the map at this point and are now discarding the copy.
        // This is fine, because this case should be really rare.
        return boost::none;
    }
    auto oldEmotePtr = it->second;
    // copy the existing emote, to not change the original one
    auto emote = *oldEmotePtr;
    updatedMap.erase(it);

    // Step 2: update the emote
    if (!updateChannelEmote(emote, channelDisplayName, message.jsonEmote))
    {
        // The emote wasn't actually updated
        return boost::none;
    }

    auto name = emote.name;
    auto emotePtr = std::make_shared<const Emote>(std::move(emote));
    updatedMap[name] = emotePtr;
    channelEmoteMap.set(std::make_shared<EmoteMap>(std::move(updatedMap)));

    return std::make_pair(oldEmotePtr, emotePtr);
}

boost::optional<EmotePtr> BttvEmotes::removeEmote(
    Atomic<std::shared_ptr<const EmoteMap>> &channelEmoteMap,
    const BttvLiveUpdateEmoteRemoveMessage &message)
{
    // This copies the map.
    EmoteMap updatedMap = *channelEmoteMap.get();
    auto it = updatedMap.findEmote(QString(), message.emoteID);
    if (it == updatedMap.end())
    {
        // We already copied the map at this point and are now discarding the copy.
        // This is fine, because this case should be really rare.
        return boost::none;
    }
    auto emote = it->second;
    updatedMap.erase(it);
    channelEmoteMap.set(std::make_shared<EmoteMap>(std::move(updatedMap)));

    return emote;
}

/*
static Url getEmoteLink(QString urlTemplate, const EmoteId &id,
                        const QString &emoteScale)
{
    urlTemplate.detach();

    return {urlTemplate.replace("{{id}}", id.string)
                .replace("{{image}}", emoteScale)};
}
*/

}  // namespace chatterino
