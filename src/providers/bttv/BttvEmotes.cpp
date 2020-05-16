#include "providers/bttv/BttvEmotes.hpp"

#include <QJsonArray>
#include <QThread>

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino {
namespace {

    QString emoteLinkFormat("https://betterttv.com/emotes/%1");

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
                Tooltip{name.string + "<br />Global BetterTTV Emote"},
                Url{emoteLinkFormat.arg(id.string)},
            });

            emotes[name] =
                cachedOrMakeEmotePtr(std::move(emote), currentEmotes);
        }

        return {Success, std::move(emotes)};
    }
    std::pair<Outcome, EmoteMap> parseChannelEmotes(const QJsonObject &jsonRoot)
    {
        auto emotes = EmoteMap();

        auto innerParse = [&jsonRoot, &emotes](const char *key) {
            auto jsonEmotes = jsonRoot.value(key).toArray();
            for (auto jsonEmote_ : jsonEmotes)
            {
                auto jsonEmote = jsonEmote_.toObject();

                auto id = EmoteId{jsonEmote.value("id").toString()};
                auto name = EmoteName{jsonEmote.value("code").toString()};
                // emoteObject.value("imageType").toString();

                auto emote = Emote({
                    name,
                    ImageSet{
                        Image::fromUrl(getEmoteLinkV3(id, "1x"), 1),
                        Image::fromUrl(getEmoteLinkV3(id, "2x"), 0.5),
                        Image::fromUrl(getEmoteLinkV3(id, "3x"), 0.25),
                    },
                    Tooltip{name.string + "<br />Channel BetterTTV Emote"},
                    Url{emoteLinkFormat.arg(id.string)},
                });

                emotes[name] = cachedOrMake(std::move(emote), id);
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
    NetworkRequest(QString(globalEmoteApiUrl))
        .timeout(30000)
        .onSuccess([this](auto result) -> Outcome {
            auto emotes = this->global_.get();
            auto pair = parseGlobalEmotes(result.parseJsonArray(), *emotes);
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        })
        .execute();
}

void BttvEmotes::loadChannel(std::weak_ptr<Channel> channel,
                             const QString &channelId,
                             std::function<void(EmoteMap &&)> callback,
                             bool manualRefresh)
{
    NetworkRequest(QString(bttvChannelEmoteApiUrl) + channelId)
        .timeout(3000)
        .onSuccess([callback = std::move(callback), channel,
                    manualRefresh](auto result) -> Outcome {
            auto pair = parseChannelEmotes(result.parseJson());
            if (pair.first)
                callback(std::move(pair.second));
            if (auto shared = channel.lock(); manualRefresh)
                shared->addMessage(
                    makeSystemMessage("BetterTTV channel emotes reloaded."));
            return pair.first;
        })
        .onError([channelId, channel, manualRefresh](auto result) {
            auto shared = channel.lock();
            if (!shared)
                return;
            if (result.status() == 203)
            {
                // User does not have any BTTV emotes
                if (manualRefresh)
                    shared->addMessage(makeSystemMessage(
                        "This channel has no BetterTTV channel emotes."));
            }
            else if (result.status() == NetworkResult::timedoutStatus)
            {
                // TODO: Auto retry in case of a timeout, with a delay
                qDebug() << "Fetching BTTV emotes for channel" << channelId
                         << "failed due to timeout";
                shared->addMessage(makeSystemMessage(
                    "Failed to fetch BetterTTV channel emotes. (timed out)"));
            }
            else
            {
                qDebug() << "Error fetching BTTV emotes for channel"
                         << channelId << ", error" << result.status();
                shared->addMessage(
                    makeSystemMessage("Failed to fetch BetterTTV channel "
                                      "emotes. (unknown error)"));
            }
        })
        .execute();
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
