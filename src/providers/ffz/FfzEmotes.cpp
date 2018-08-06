#include "providers/ffz/FfzEmotes.hpp"

#include <QJsonArray>

#include "common/NetworkRequest.hpp"
#include "debug/Log.hpp"
#include "messages/Image.hpp"

namespace chatterino {
namespace {
Url getEmoteLink(const QJsonObject &urls, const QString &emoteScale)
{
    auto emote = urls.value(emoteScale);
    if (emote.isUndefined()) {
        return {""};
    }

    assert(emote.isString());

    return {"https:" + emote.toString()};
}

void fillInEmoteData(const QJsonObject &urls, const EmoteName &name,
                     const QString &tooltip, Emote &emoteData)
{
    auto url1x = getEmoteLink(urls, "1");
    auto url2x = getEmoteLink(urls, "2");
    auto url3x = getEmoteLink(urls, "4");

    //, code, tooltip
    emoteData.name = name;
    emoteData.images =
        ImageSet{Image::fromUrl(url1x, 1), Image::fromUrl(url2x, 0.5),
                 Image::fromUrl(url3x, 0.25)};
    emoteData.tooltip = {tooltip};
}
}  // namespace

AccessGuard<const EmoteCache<EmoteName>> FfzEmotes::accessGlobalEmotes() const
{
    return this->globalEmotes_.accessConst();
}

boost::optional<EmotePtr> FfzEmotes::getEmote(const EmoteId &id)
{
    auto cache = this->channelEmoteCache_.access();
    auto it = cache->find(id);

    if (it != cache->end()) {
        auto shared = it->second.lock();
        if (shared) {
            return shared;
        }
    }

    return boost::none;
}

boost::optional<EmotePtr> FfzEmotes::getGlobalEmote(const EmoteName &name)
{
    return this->globalEmotes_.access()->get(name);
}

void FfzEmotes::loadGlobalEmotes()
{
    QString url("https://api.frankerfacez.com/v1/set/global");

    NetworkRequest request(url);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([this](auto result) -> Outcome {
        return this->parseGlobalEmotes(result.parseJson());
    });

    request.execute();
}

Outcome FfzEmotes::parseGlobalEmotes(const QJsonObject &jsonRoot)
{
    auto jsonSets = jsonRoot.value("sets").toObject();
    auto emotes = this->globalEmotes_.access();
    auto replacement = emotes->makeReplacment();

    for (auto jsonSet : jsonSets) {
        auto jsonEmotes = jsonSet.toObject().value("emoticons").toArray();

        for (auto jsonEmoteValue : jsonEmotes) {
            auto jsonEmote = jsonEmoteValue.toObject();

            auto name = EmoteName{jsonEmote.value("name").toString()};
            auto id = EmoteId{jsonEmote.value("id").toString()};
            auto urls = jsonEmote.value("urls").toObject();

            auto emote = Emote();
            fillInEmoteData(urls, name, name.string + "<br/>Global FFZ Emote",
                            emote);
            emote.homePage =
                Url{QString("https://www.frankerfacez.com/emoticon/%1-%2")
                        .arg(id.string)
                        .arg(name.string)};

            replacement.add(name, emote);
        }
    }

    return Success;
}

void FfzEmotes::loadChannelEmotes(const QString &channelName,
                                  std::function<void(EmoteMap &&)> callback)
{
    // printf("[FFZEmotes] Reload FFZ Channel Emotes for channel %s\n",
    // qPrintable(channelName));

    // QString url("https://api.frankerfacez.com/v1/room/" + channelName);

    // NetworkRequest request(url);
    // request.setCaller(QThread::currentThread());
    // request.setTimeout(3000);
    // request.onSuccess([this, channelName, _map](auto result) -> Outcome {
    //    return this->parseChannelEmotes(result.parseJson());
    //});

    // request.execute();
}

Outcome parseChannelEmotes(const QJsonObject &jsonRoot)
{
    // auto rootNode = result.parseJson();
    // auto map = _map.lock();

    // if (_map.expired()) {
    //    return false;
    //}

    // map->clear();

    // auto setsNode = rootNode.value("sets").toObject();

    // std::vector<QString> codes;
    // for (const QJsonValue &setNode : setsNode) {
    //    auto emotesNode = setNode.toObject().value("emoticons").toArray();

    //    for (const QJsonValue &emoteNode : emotesNode) {
    //        QJsonObject emoteObject = emoteNode.toObject();

    //        // margins
    //        int id = emoteObject.value("id").toInt();
    //        QString code = emoteObject.value("name").toString();

    //        QJsonObject urls = emoteObject.value("urls").toObject();

    //        auto emote = this->channelEmoteCache_.getOrAdd(id, [id, &code,
    //        &urls] {
    //            EmoteData emoteData;
    //            fillInEmoteData(urls, code, code + "<br/>Channel FFZ Emote",
    //            emoteData); emoteData.pageLink =
    //                QString("https://www.frankerfacez.com/emoticon/%1-%2").arg(id).arg(code);

    //            return emoteData;
    //        });

    //        this->channelEmotes.insert(code, emote);
    //        map->insert(code, emote);
    //        codes.push_back(code);
    //    }

    //    this->channelEmoteCodes[channelName] = codes;
    //}

    return Success;
}

}  // namespace chatterino
