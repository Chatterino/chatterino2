#include "providers/ffz/FfzEmotes.hpp"

#include "common/UrlFetch.hpp"
#include "debug/Log.hpp"
#include "messages/Image.hpp"

namespace chatterino {

namespace {

QString getEmoteLink(const QJsonObject &urls, const QString &emoteScale)
{
    auto emote = urls.value(emoteScale);
    if (emote.isUndefined()) {
        return "";
    }

    assert(emote.isString());

    return "https:" + emote.toString();
}

void fillInEmoteData(const QJsonObject &urls, const QString &code, const QString &tooltip,
                     EmoteData &emoteData)
{
    QString url1x = getEmoteLink(urls, "1");
    QString url2x = getEmoteLink(urls, "2");
    QString url3x = getEmoteLink(urls, "4");

    assert(!url1x.isEmpty());

    emoteData.image1x = new Image(url1x, 1, code, tooltip);

    if (!url2x.isEmpty()) {
        emoteData.image2x = new Image(url2x, 0.5, code, tooltip);
    }

    if (!url3x.isEmpty()) {
        emoteData.image3x = new Image(url3x, 0.25, code, tooltip);
    }
}

}  // namespace

void FFZEmotes::loadGlobalEmotes()
{
    QString url("https://api.frankerfacez.com/v1/set/global");

    NetworkRequest request(url);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([this](auto result) {
        auto root = result.parseJson();
        auto sets = root.value("sets").toObject();

        std::vector<QString> codes;
        for (const QJsonValue &set : sets) {
            auto emoticons = set.toObject().value("emoticons").toArray();

            for (const QJsonValue &emote : emoticons) {
                QJsonObject object = emote.toObject();

                QString code = object.value("name").toString();
                int id = object.value("id").toInt();
                QJsonObject urls = object.value("urls").toObject();

                EmoteData emoteData;
                fillInEmoteData(urls, code, code + "<br/>Global FFZ Emote", emoteData);
                emoteData.pageLink =
                    QString("https://www.frankerfacez.com/emoticon/%1-%2").arg(id).arg(code);

                this->globalEmotes.insert(code, emoteData);
                codes.push_back(code);
            }

            this->globalEmoteCodes = codes;
        }

        return true;
    });

    request.execute();
}

void FFZEmotes::loadChannelEmotes(const QString &channelName, std::weak_ptr<EmoteMap> _map)
{
    printf("[FFZEmotes] Reload FFZ Channel Emotes for channel %s\n", qPrintable(channelName));

    QString url("https://api.frankerfacez.com/v1/room/" + channelName);

    NetworkRequest request(url);
    request.setCaller(QThread::currentThread());
    request.setTimeout(3000);
    request.onSuccess([this, channelName, _map](auto result) {
        auto rootNode = result.parseJson();
        auto map = _map.lock();

        if (_map.expired()) {
            return false;
        }

        map->clear();

        auto setsNode = rootNode.value("sets").toObject();

        std::vector<QString> codes;
        for (const QJsonValue &setNode : setsNode) {
            auto emotesNode = setNode.toObject().value("emoticons").toArray();

            for (const QJsonValue &emoteNode : emotesNode) {
                QJsonObject emoteObject = emoteNode.toObject();

                // margins
                int id = emoteObject.value("id").toInt();
                QString code = emoteObject.value("name").toString();

                QJsonObject urls = emoteObject.value("urls").toObject();

                auto emote = this->channelEmoteCache_.getOrAdd(id, [id, &code, &urls] {
                    EmoteData emoteData;
                    fillInEmoteData(urls, code, code + "<br/>Channel FFZ Emote", emoteData);
                    emoteData.pageLink =
                        QString("https://www.frankerfacez.com/emoticon/%1-%2").arg(id).arg(code);

                    return emoteData;
                });

                this->channelEmotes.insert(code, emote);
                map->insert(code, emote);
                codes.push_back(code);
            }

            this->channelEmoteCodes[channelName] = codes;
        }

        return true;
    });

    request.execute();
}

}  // namespace chatterino
