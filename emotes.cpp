#include "emotes.h"
#include "resources.h"

QString Emotes::m_twitchEmoteTemplate(
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}.0");

ConcurrentMap<QString, TwitchEmoteValue *> Emotes::m_twitchEmotes;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_bttvEmotes;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_ffzEmotes;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_chatterinoEmotes;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_bttvChannelEmoteFromCaches;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_ffzChannelEmoteFromCaches;
ConcurrentMap<long, LazyLoadedImage *> Emotes::m_twitchEmoteFromCache;
ConcurrentMap<QString, LazyLoadedImage *> Emotes::m_miscImageFromCache;

int Emotes::m_generation = 0;

Emotes::Emotes()
{
}

LazyLoadedImage *
Emotes::getTwitchEmoteById(const QString &name, long id)
{
    return m_twitchEmoteFromCache.getOrAdd(id, [&name, id] {
        qreal scale;
        QString url = getTwitchEmoteLink(id, scale);

        return new LazyLoadedImage(url, scale, name, name + "\nTwitch Emote");
    });
}

QString
Emotes::getTwitchEmoteLink(long id, qreal &scale)
{
    scale = .5;

    return m_twitchEmoteTemplate.replace("{id}", QString::number(id))
        .replace("{scale}", "2");
}

LazyLoadedImage *
Emotes::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return getCheerBadge(amount);
}

LazyLoadedImage *
Emotes::getCheerBadge(long long amount)
{
    if (amount >= 100000) {
        return Resources::cheerBadge100000();
    } else if (amount >= 10000) {
        return Resources::cheerBadge10000();
    } else if (amount >= 5000) {
        return Resources::cheerBadge5000();
    } else if (amount >= 1000) {
        return Resources::cheerBadge1000();
    } else if (amount >= 100) {
        return Resources::cheerBadge100();
    } else {
        return Resources::cheerBadge1();
    }
}
