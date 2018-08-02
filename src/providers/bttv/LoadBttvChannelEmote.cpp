#include "LoadBttvChannelEmote.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

static Url getEmoteLink(QString urlTemplate, const EmoteId &id, const QString &emoteScale);
static std::pair<Outcome, EmoteMap> bttvParseChannelEmotes(const QJsonObject &jsonRoot);

void loadBttvChannelEmotes(const QString &channelName, std::function<void(EmoteMap &&)> callback)
{
    auto request = NetworkRequest(QString(bttvChannelEmoteApiUrl) + channelName);

    request.setCaller(QThread::currentThread());
    request.setTimeout(3000);
    request.onSuccess([callback = std::move(callback)](auto result) -> Outcome {
        auto pair = bttvParseChannelEmotes(result.parseJson());

        if (pair.first == Success) callback(std::move(pair.second));

        return pair.first;
    });

    request.execute();
}

static std::pair<Outcome, EmoteMap> bttvParseChannelEmotes(const QJsonObject &jsonRoot)
{
    static UniqueAccess<std::unordered_map<EmoteId, std::weak_ptr<const Emote>>> cache_;

    auto cache = cache_.access();
    auto emotes = EmoteMap();
    auto jsonEmotes = jsonRoot.value("emotes").toArray();
    auto urlTemplate = QString("https:" + jsonRoot.value("urlTemplate").toString());

    for (auto jsonEmote_ : jsonEmotes) {
        auto jsonEmote = jsonEmote_.toObject();

        auto id = EmoteId{jsonEmote.value("id").toString()};
        auto name = EmoteName{jsonEmote.value("code").toString()};
        // emoteObject.value("imageType").toString();

        auto emote = Emote({name,
                            ImageSet{Image::fromUrl(getEmoteLink(urlTemplate, id, "1x"), 1),
                                     Image::fromUrl(getEmoteLink(urlTemplate, id, "2x"), 0.5),
                                     Image::fromUrl(getEmoteLink(urlTemplate, id, "3x"), 0.25)},
                            Tooltip{name.string + "<br />Channel Bttv Emote"},
                            Url{"https://manage.betterttv.net/emotes/" + id.string}});

        auto shared = (*cache)[id].lock();
        if (shared && *shared == emote) {
            // reuse old shared_ptr if nothing changed
            emotes[name] = shared;
        } else {
            (*cache)[id] = emotes[name] = std::make_shared<Emote>(std::move(emote));
        }
    }

    return {Success, std::move(emotes)};
}

static Url getEmoteLink(QString urlTemplate, const EmoteId &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return {urlTemplate.replace("{{id}}", id.string).replace("{{image}}", emoteScale)};
}

}  // namespace chatterino
