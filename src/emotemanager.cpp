#include "emotemanager.h"
#include "resources.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

#define TWITCH_EMOTE_TEMPLATE "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}.0"

using namespace chatterino::messages;

namespace chatterino {

EmoteManager EmoteManager::instance;

EmoteManager::EmoteManager()
    : _twitchEmotes()
    , _bttvEmotes()
    , _ffzEmotes()
    , _chatterinoEmotes()
    , _bttvChannelEmoteFromCaches()
    , _ffzChannelEmoteFromCaches()
    , _twitchEmoteFromCache()
    , _miscImageFromCache()
    , _gifUpdateTimerSignal()
    , _gifUpdateTimer()
    , _gifUpdateTimerInitiated(false)
    , _generation(0)
{
}

ConcurrentMap<QString, twitch::EmoteValue *> &EmoteManager::getTwitchEmotes()
{
    return _twitchEmotes;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &EmoteManager::getBttvEmotes()
{
    return _bttvEmotes;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &EmoteManager::getFfzEmotes()
{
    return _ffzEmotes;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &EmoteManager::getChatterinoEmotes()
{
    return _chatterinoEmotes;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &EmoteManager::getBttvChannelEmoteFromCaches()
{
    return _bttvChannelEmoteFromCaches;
}

ConcurrentMap<int, messages::LazyLoadedImage *> &EmoteManager::getFfzChannelEmoteFromCaches()
{
    return _ffzChannelEmoteFromCaches;
}

ConcurrentMap<long, messages::LazyLoadedImage *> &EmoteManager::getTwitchEmoteFromCache()
{
    return _twitchEmoteFromCache;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &EmoteManager::getMiscImageFromCache()
{
    return _miscImageFromCache;
}

void EmoteManager::loadGlobalEmotes()
{
    loadBttvEmotes();
    loadFfzEmotes();
}

void EmoteManager::loadBttvEmotes()
{
    // bttv
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url("https://api.betterttv.net/2/emotes");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            auto emotes = root.value("emotes").toArray();

            QString linkTemplate = "https:" + root.value("urlTemplate").toString();

            for (const QJsonValue &emote : emotes) {
                QString id = emote.toObject().value("id").toString();
                QString code = emote.toObject().value("code").toString();
                // emote.value("imageType").toString();

                QString tmp = linkTemplate;
                tmp.detach();
                QString url = tmp.replace("{{id}}", id).replace("{{image}}", "1x");

                EmoteManager::getBttvEmotes().insert(
                    code, new LazyLoadedImage(url, 1, code, code + "\nGlobal Bttv Emote"));
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

void EmoteManager::loadFfzEmotes()
{
    // ffz
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url("https://api.frankerfacez.com/v1/set/global");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
            QJsonObject root = jsonDoc.object();

            auto sets = root.value("sets").toObject();

            for (const QJsonValue &set : sets) {
                auto emoticons = set.toObject().value("emoticons").toArray();

                for (const QJsonValue &emote : emoticons) {
                    QJsonObject object = emote.toObject();

                    // margins

                    // int id = object.value("id").toInt();
                    QString code = object.value("name").toString();

                    QJsonObject urls = object.value("urls").toObject();
                    QString url1 = "http:" + urls.value("1").toString();

                    EmoteManager::getBttvEmotes().insert(
                        code, new LazyLoadedImage(url1, 1, code, code + "\nGlobal Ffz Emote"));
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

LazyLoadedImage *EmoteManager::getTwitchEmoteById(const QString &name, long id)
{
    return EmoteManager::_twitchEmoteFromCache.getOrAdd(id, [&name, &id] {
        qDebug() << "added twitch emote: " << id;
        qreal scale;
        QString url = getTwitchEmoteLink(id, scale);
        return new LazyLoadedImage(url, scale, name, name + "\nTwitch Emote");
    });
}

QString EmoteManager::getTwitchEmoteLink(long id, qreal &scale)
{
    scale = .5;

    QString value = TWITCH_EMOTE_TEMPLATE;

    value.detach();

    return value.replace("{id}", QString::number(id)).replace("{scale}", "2");
}

LazyLoadedImage *EmoteManager::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return getCheerBadge(amount);
}

LazyLoadedImage *EmoteManager::getCheerBadge(long long amount)
{
    if (amount >= 100000) {
        return Resources::getCheerBadge100000();
    } else if (amount >= 10000) {
        return Resources::getCheerBadge10000();
    } else if (amount >= 5000) {
        return Resources::getCheerBadge5000();
    } else if (amount >= 1000) {
        return Resources::getCheerBadge1000();
    } else if (amount >= 100) {
        return Resources::getCheerBadge100();
    } else {
        return Resources::getCheerBadge1();
    }
}

}  // namespace chatterino
