#include "providers/bttv/BttvEmotes.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "util/Log.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QJsonArray>
#include <QThread>

namespace chatterino
{
    namespace
    {
        Url getEmoteLink(
            QString urlTemplate, const EmoteId& id, const QString& emoteScale)
        {
            urlTemplate.detach();

            return {urlTemplate.replace("{{id}}", id.string)
                        .replace("{{image}}", emoteScale)};
        }
        std::pair<Outcome, EmoteMap> parseGlobalEmotes(
            const QJsonObject& jsonRoot, const EmoteMap& currentEmotes)
        {
            auto emotes = EmoteMap();
            auto jsonEmotes = jsonRoot.value("emotes").toArray();
            auto urlTemplate =
                qS("https:") + jsonRoot.value("urlTemplate").toString();

            for (auto jsonEmote : jsonEmotes)
            {
                auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
                auto name =
                    EmoteName{jsonEmote.toObject().value("code").toString()};

                auto emote = Emote({name,
                    ImageSet{
                        Image::fromUrl(getEmoteLink(urlTemplate, id, "1x"), 1),
                        Image::fromUrl(
                            getEmoteLink(urlTemplate, id, "2x"), 0.5),
                        Image::fromUrl(
                            getEmoteLink(urlTemplate, id, "3x"), 0.25)},
                    Tooltip{name.string + "<br />Global BetterTTV Emote"},
                    Url{"https://manage.betterttv.net/emotes/" + id.string}});

                emotes[name] =
                    cachedOrMakeEmotePtr(std::move(emote), currentEmotes);
            }

            return {Success, std::move(emotes)};
        }
        EmotePtr cachedOrMake(Emote&& emote, const EmoteId& id)
        {
            static std::unordered_map<EmoteId, std::weak_ptr<const Emote>>
                cache;
            static std::mutex mutex;

            return cachedOrMakeEmotePtr(std::move(emote), cache, mutex, id);
        }
        std::pair<Outcome, EmoteMap> parseChannelEmotes(
            const QJsonObject& jsonRoot)
        {
            auto emotes = EmoteMap();
            auto jsonEmotes = jsonRoot.value("emotes").toArray();
            auto urlTemplate =
                "https:" + jsonRoot.value("urlTemplate").toString();

            for (auto jsonEmote_ : jsonEmotes)
            {
                auto jsonEmote = jsonEmote_.toObject();

                auto id = EmoteId{jsonEmote.value("id").toString()};
                auto name = EmoteName{jsonEmote.value("code").toString()};
                // emoteObject.value("imageType").toString();

                auto emote = Emote({name,
                    ImageSet{
                        Image::fromUrl(getEmoteLink(urlTemplate, id, "1x"), 1),
                        Image::fromUrl(
                            getEmoteLink(urlTemplate, id, "2x"), 0.5),
                        Image::fromUrl(
                            getEmoteLink(urlTemplate, id, "3x"), 0.25)},
                    Tooltip{name.string + "<br />Channel BetterTTV Emote"},
                    Url{"https://manage.betterttv.net/emotes/" + id.string}});

                emotes[name] = cachedOrMake(std::move(emote), id);
            }

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

    boost::optional<EmotePtr> BttvEmotes::emote(const EmoteName& name) const
    {
        auto emotes = this->global_.get();
        auto it = emotes->find(name);

        if (it == emotes->end())
            return boost::none;
        return it->second;
    }

    void BttvEmotes::loadEmotes()
    {
        auto request = NetworkRequest(QString(globalEmoteApiUrl));

        request.setCaller(QThread::currentThread());
        request.setTimeout(30000);

        request.onSuccess([this](auto result) -> Outcome {
            auto emotes = this->global_.get();
            auto pair = parseGlobalEmotes(result.parseJson(), *emotes);
            if (pair.first)
                this->global_.set(
                    std::make_shared<EmoteMap>(std::move(pair.second)));
            return pair.first;
        });

        request.execute();
    }

    void BttvEmotes::loadChannel(
        const QString& channelName, std::function<void(EmoteMap&&)> callback)
    {
        auto request =
            NetworkRequest(QString(bttvChannelEmoteApiUrl) + channelName);

        request.setCaller(QThread::currentThread());
        request.setTimeout(3000);

        request.onSuccess(
            [callback = std::move(callback)](auto result) -> Outcome {
                auto pair = parseChannelEmotes(result.parseJson());
                if (pair.first)
                    callback(std::move(pair.second));
                return pair.first;
            });

        request.execute();
    }

    static Url getEmoteLink(
        QString urlTemplate, const EmoteId& id, const QString& emoteScale)
    {
        urlTemplate.detach();

        return {urlTemplate.replace("{{id}}", id.string)
                    .replace("{{image}}", emoteScale)};
    }

}  // namespace chatterino
