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

void BttvEmotes::loadChannel(TwitchChannel &channel, const QString &channelId,
                             std::function<void(EmoteMap &&)> callback)
{
    NetworkRequest(QString(bttvChannelEmoteApiUrl) + channelId)
        .timeout(3000)
        .onSuccess(
            [callback = std::move(callback), &channel](auto result) -> Outcome {
                auto pair = parseChannelEmotes(result.parseJson());
                if (pair.first)
                    callback(std::move(pair.second));
                channel.addMessage(makeSystemMessage("BTTV: emotes reloaded."));
                return pair.first;
            })
        .onError([channelId, &channel](auto result) {
            if (result.status() == 203)
            {
                // User does not have any BTTV emotes
                channel.addMessage(
                    makeSystemMessage("BTTV: this channel has no emotes."));
            }
            else if (result.status() == NetworkResult::timedoutStatus)
            {
                // TODO: Auto retry in case of a timeout, with a delay
                qDebug() << "Fetching BTTV emotes for channel" << channelId
                         << "failed due to timeout";
                channel.addMessage(makeSystemMessage(
                    "BTTV: failed to fetch emotes. (timed out)"));
            }
            else
            {
                qDebug() << "Error fetching BTTV emotes for channel"
                         << channelId << ", error" << result.status();
                channel.addMessage(makeSystemMessage(
                    "BTTV: failed to fetch emotes. (unknown error)"));
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
