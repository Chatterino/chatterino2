#include "emotes.h"
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

namespace chatterino {

QString Emotes::twitchEmoteTemplate(
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}.0");

ConcurrentMap<QString, TwitchEmoteValue *> Emotes::twitchEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::bttvEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::ffzEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::chatterinoEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *>
    Emotes::bttvChannelEmoteFromCaches;
ConcurrentMap<int, messages::LazyLoadedImage *>
    Emotes::ffzChannelEmoteFromCaches;
ConcurrentMap<long, messages::LazyLoadedImage *> Emotes::twitchEmoteFromCache;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::miscImageFromCache;
boost::signals2::signal<void()> Emotes::gifUpdateTimerSignal;

QTimer Emotes::gifUpdateTimer;
bool Emotes::gifUpdateTimerInitiated(false);

int Emotes::generation = 0;

Emotes::Emotes()
{
}

void
Emotes::loadGlobalEmotes()
{
    loadBttvEmotes();
    loadFfzEmotes();
}

void
Emotes::loadBttvEmotes()
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

            QString _template = "https:" + root.value("urlTemplate").toString();

            for (const QJsonValue &emote : emotes) {
                QString id = emote.toObject().value("id").toString();
                QString code = emote.toObject().value("code").toString();
                // emote.value("imageType").toString();

                QString tmp = _template;
                tmp.detach();
                QString url =
                    tmp.replace("{{id}}", id).replace("{{image}}", "1x");

                Emotes::getBttvEmotes().insert(
                    code, new messages::LazyLoadedImage(
                              url, 1, code, code + "\nGlobal Bttv Emote"));
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

void
Emotes::loadFfzEmotes()
{
    // bttv
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

                    int id = object.value("id").toInt();
                    QString code = object.value("name").toString();

                    QJsonObject urls = object.value("urls").toObject();
                    QString url1 = "http:" + urls.value("1").toString();

                    Emotes::getBttvEmotes().insert(
                        code, new messages::LazyLoadedImage(
                                  url1, 1, code, code + "\nGlobal Ffz Emote"));
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

messages::LazyLoadedImage *
Emotes::getTwitchEmoteById(const QString &name, long id)
{
    qDebug() << "loading twitch emote: " << id;

    return Emotes::twitchEmoteFromCache.getOrAdd(id, [&name, &id] {
        qDebug() << "loading twitch emote: " << id;
        qreal scale;
        QString url = getTwitchEmoteLink(id, scale);
        return new messages::LazyLoadedImage(url, scale, name,
                                             name + "\nTwitch Emote");
    });
}

QString
Emotes::getTwitchEmoteLink(long id, qreal &scale)
{
    scale = .5;

    QString value = Emotes::twitchEmoteTemplate;

    value.detach();

    return value.replace("{id}", QString::number(id)).replace("{scale}", "2");
}

messages::LazyLoadedImage *
Emotes::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return getCheerBadge(amount);
}

messages::LazyLoadedImage *
Emotes::getCheerBadge(long long amount)
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
}
