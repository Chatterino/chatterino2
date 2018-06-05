#include "providers/bttv/bttvemotes.hpp"

#include "debug/log.hpp"
#include "messages/image.hpp"
#include "util/urlfetch.hpp"

namespace chatterino {
namespace providers {
namespace bttv {

namespace {

QString getEmoteLink(QString urlTemplate, const QString &id, const QString &emoteScale)
{
    urlTemplate.detach();

    return urlTemplate.replace("{{id}}", id).replace("{{image}}", emoteScale);
}

}  // namespace

util::EmoteMap &BTTVEmotes::getBTTVChannelEmoteFromCaches()
{
    return _bttvChannelEmoteFromCaches;
}

void BTTVEmotes::loadGlobalEmotes()
{
    QString url("https://api.betterttv.net/2/emotes");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(30000);
    req.setUseQuickLoadCache(true);
    req.getJSON([this](QJsonObject &root) {
        auto emotes = root.value("emotes").toArray();

        QString urlTemplate = "https:" + root.value("urlTemplate").toString();

        std::vector<std::string> codes;
        for (const QJsonValue &emote : emotes) {
            QString id = emote.toObject().value("id").toString();
            QString code = emote.toObject().value("code").toString();

            util::EmoteData emoteData;
            emoteData.image1x = new messages::Image(getEmoteLink(urlTemplate, id, "1x"), 1, code,
                                                    code + "<br />Global BTTV Emote");
            emoteData.image2x = new messages::Image(getEmoteLink(urlTemplate, id, "2x"), 0.5, code,
                                                    code + "<br />Global BTTV Emote");
            emoteData.image3x = new messages::Image(getEmoteLink(urlTemplate, id, "3x"), 0.25, code,
                                                    code + "<br />Global BTTV Emote");
            emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

            this->globalEmotes.insert(code, emoteData);
            codes.push_back(code.toStdString());
        }

        this->globalEmoteCodes = codes;
    });
}

void BTTVEmotes::loadChannelEmotes(const QString &channelName, std::weak_ptr<util::EmoteMap> _map)
{
    printf("[BTTVEmotes] Reload BTTV Channel Emotes for channel %s\n", qPrintable(channelName));

    QString url("https://api.betterttv.net/2/channels/" + channelName);

    debug::Log("Request bttv channel emotes for {}", channelName);

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.setTimeout(3000);
    req.getJSON([this, channelName, _map](QJsonObject &rootNode) {
        auto map = _map.lock();

        if (_map.expired()) {
            return;
        }

        map->clear();

        auto emotesNode = rootNode.value("emotes").toArray();

        QString linkTemplate = "https:" + rootNode.value("urlTemplate").toString();

        std::vector<std::string> codes;
        for (const QJsonValue &emoteNode : emotesNode) {
            QJsonObject emoteObject = emoteNode.toObject();

            QString id = emoteObject.value("id").toString();
            QString code = emoteObject.value("code").toString();
            // emoteObject.value("imageType").toString();

            auto emote = this->getBTTVChannelEmoteFromCaches().getOrAdd(id, [&] {
                util::EmoteData emoteData;
                QString link = linkTemplate;
                link.detach();
                emoteData.image1x =
                    new messages::Image(link.replace("{{id}}", id).replace("{{image}}", "1x"), 1,
                                        code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image2x =
                    new messages::Image(link.replace("{{id}}", id).replace("{{image}}", "2x"), 0.5,
                                        code, code + "<br />Channel BTTV Emote");
                link = linkTemplate;
                link.detach();
                emoteData.image3x =
                    new messages::Image(link.replace("{{id}}", id).replace("{{image}}", "3x"), 0.25,
                                        code, code + "<br />Channel BTTV Emote");
                emoteData.pageLink = "https://manage.betterttv.net/emotes/" + id;

                return emoteData;
            });

            this->channelEmotes.insert(code, emote);
            map->insert(code, emote);
            codes.push_back(code.toStdString());
        }

        this->channelEmoteCodes[channelName.toStdString()] = codes;
    });
}

}  // namespace bttv
}  // namespace providers
}  // namespace chatterino
