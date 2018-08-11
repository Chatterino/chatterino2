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
std::pair<Outcome, EmoteMap> parseGlobalEmotes(const QJsonObject &jsonRoot,
                                               const EmoteMap &currentEmotes)
{
    auto jsonSets = jsonRoot.value("sets").toObject();
    auto emotes = EmoteMap();

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

            emotes[name] =
                cachedOrMakeEmotePtr(std::move(emote), currentEmotes);
        }
    }

    return {Success, std::move(emotes)};
}
}  // namespace

FfzEmotes::FfzEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> FfzEmotes::global() const
{
    return this->global_.get();
}

boost::optional<EmotePtr> FfzEmotes::global(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);
    if (it != emotes->end()) return it->second;
    return boost::none;
}

void FfzEmotes::loadGlobal()
{
    QString url("https://api.frankerfacez.com/v1/set/global");

    NetworkRequest request(url);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);

    request.onSuccess([this](auto result) -> Outcome {
        auto emotes = this->global();
        auto pair = parseGlobalEmotes(result.parseJson(), *emotes);
        if (pair.first)
            this->global_.set(
                std::make_shared<EmoteMap>(std::move(pair.second)));
        return pair.first;
    });

    request.execute();
}

void FfzEmotes::loadChannel(const QString &channelName,
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
