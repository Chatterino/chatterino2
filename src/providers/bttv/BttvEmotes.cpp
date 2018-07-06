#include "providers/bttv/BttvEmotes.hpp"

#include "common/UrlFetch.hpp"
#include "debug/Log.hpp"
#include "messages/Image.hpp"

namespace chatterino {

namespace {

QString getEmoteLink(QString urlTemplate, const QString &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return urlTemplate.replace("{{id}}", id).replace("{{image}}", emoteScale);
}

}  // namespace

void BTTVEmotes::loadGlobalEmotes()
{
    QString url("https://api.betterttv.net/2/emotes");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(30000);
    req.setUseQuickLoadCache(true);
    req.getJSON([this](QJsonObject &root) {
        auto emotes = root.value("emotes").toArray();

        QString urlTemplate = "https:" + root.value("urlTemplate").toString();

        std::vector<QString> codes;
        for (const QJsonValue &emote : emotes) {
            QString id = emote.toObject().value("id").toString();
            QString code = emote.toObject().value("code").toString();

            EmoteData emoteData;
            emoteData.image1x = new Image(getEmoteLink(urlTemplate, id, "1x"), 1, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.image2x = new Image(getEmoteLink(urlTemplate, id, "2x"), 0.5, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.image3x = new Image(getEmoteLink(urlTemplate, id, "3x"), 0.25, code,
                                          code + "<br />Global BTTV Emote");
            emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

            this->globalEmotes.insert(code, emoteData);
            codes.push_back(code);
        }

        this->globalEmoteCodes = codes;
    });
}

void BTTVEmotes::loadChannelEmotes(const QString &channelName, std::weak_ptr<EmoteMap> _map)
{
    printf("[BTTVEmotes] Reload BTTV Channel Emotes for channel %s\n", qPrintable(channelName));

    QString url("https://api.betterttv.net/2/channels/" + channelName);

    Log("Request bttv channel emotes for {}", channelName);

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(3000);
    req.setUseQuickLoadCache(true);
    req.getJSON([this, channelName, _map](QJsonObject &rootNode) {
        auto map = _map.lock();

        if (_map.expired()) {
            return;
        }

        map->clear();

        auto emotesNode = rootNode.value("emotes").toArray();

        QString linkTemplate = "https:" + rootNode.value("urlTemplate").toString();

        std::vector<QString> codes;
        for (const QJsonValue &emoteNode : emotesNode) {
            QJsonObject emoteObject = emoteNode.toObject();

            QString id = emoteObject.value("id").toString();
            QString code = emoteObject.value("code").toString();
            // emoteObject.value("imageType").toString();

            auto emote = this->channelEmoteCache_.getOrAdd(id, [&] {
                EmoteData emoteData;
                QString link = linkTemplate;
                link.detach();
                emoteData.image1x = new Image(link.replace("{{id}}", id).replace("{{image}}", "1x"),
                                              1, code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image2x = new Image(link.replace("{{id}}", id).replace("{{image}}", "2x"),
                                              0.5, code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image3x = new Image(link.replace("{{id}}", id).replace("{{image}}", "3x"),
                                              0.25, code, code + "<br />Channel BTTV Emote");
                emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

                return emoteData;
            });

            this->channelEmotes.insert(code, emote);
            map->insert(code, emote);
            codes.push_back(code);
        }

        this->channelEmoteCodes[channelName] = codes;
    });
}

}  // namespace chatterino
