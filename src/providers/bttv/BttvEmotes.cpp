#include "providers/bttv/BttvEmotes.hpp"

#include "common/NetworkRequest.hpp"
#include "debug/Log.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QJsonArray>
#include <QThread>

namespace chatterino {

namespace {

Url getEmoteLink(QString urlTemplate, const EmoteId &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return {urlTemplate.replace("{{id}}", id.string).replace("{{image}}", emoteScale)};
}

}  // namespace

AccessGuard<const EmoteMap> BttvEmotes::accessGlobalEmotes() const
{
    return this->globalEmotes_.accessConst();
}

boost::optional<EmotePtr> BttvEmotes::getGlobalEmote(const EmoteName &name)
{
    auto emotes = this->globalEmotes_.access();
    auto it = emotes->find(name);

    if (it == emotes->end()) return boost::none;
    return it->second;
}

// FOURTF: never returns anything
// boost::optional<EmotePtr> BttvEmotes::getEmote(const EmoteId &id)
//{
//    auto cache = this->channelEmoteCache_.access();
//    auto it = cache->find(id);
//
//    if (it != cache->end()) {
//        auto shared = it->second.lock();
//        if (shared) {
//            return shared;
//        }
//    }
//
//    return boost::none;
//}

void BttvEmotes::loadGlobalEmotes()
{
    auto request = NetworkRequest(QString(globalEmoteApiUrl));

    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([this](auto result) -> Outcome {
        //        if (auto shared = weak.lock()) {
        auto currentEmotes = this->globalEmotes_.access();

        auto pair = this->parseGlobalEmotes(result.parseJson(), *currentEmotes);

        if (pair.first) {
            *currentEmotes = std::move(pair.second);
        }

        return pair.first;
        //        }
        return Failure;
    });

    request.execute();
}

std::pair<Outcome, EmoteMap> BttvEmotes::parseGlobalEmotes(const QJsonObject &jsonRoot,
                                                           const EmoteMap &currentEmotes)
{
    auto emotes = EmoteMap();
    auto jsonEmotes = jsonRoot.value("emotes").toArray();
    auto urlTemplate = QString("https:" + jsonRoot.value("urlTemplate").toString());

    for (const QJsonValue &jsonEmote : jsonEmotes) {
        auto id = EmoteId{jsonEmote.toObject().value("id").toString()};
        auto name = EmoteName{jsonEmote.toObject().value("code").toString()};

        auto emote = Emote({name,
                            ImageSet{Image::fromUrl(getEmoteLink(urlTemplate, id, "1x"), 1),
                                     Image::fromUrl(getEmoteLink(urlTemplate, id, "2x"), 0.5),
                                     Image::fromUrl(getEmoteLink(urlTemplate, id, "3x"), 0.25)},
                            Tooltip{name.string + "<br />Global Bttv Emote"},
                            Url{"https://manage.betterttv.net/emotes/" + id.string}});

        auto it = currentEmotes.find(name);
        if (it != currentEmotes.end() && *it->second == emote) {
            // reuse old shared_ptr if nothing changed
            emotes[name] = it->second;
        } else {
            emotes[name] = std::make_shared<Emote>(std::move(emote));
        }
    }

    return {Success, std::move(emotes)};
}

}  // namespace chatterino
