#include "emotes.h"
#include "resources.h"

namespace chatterino {

QString Emotes::twitchEmoteTemplate(
    "https://static-cdn.jtvnw.net/emoticons/v1/{id}/{scale}.0");

ConcurrentMap<QString, TwitchEmoteValue *> Emotes::twitchEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::bttvEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::ffzEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::chatterinoEmotes;
ConcurrentMap<QString, messages::LazyLoadedImage *>
    Emotes::bttvChannelEmoteFromCaches;
ConcurrentMap<QString, messages::LazyLoadedImage *>
    Emotes::ffzChannelEmoteFromCaches;
ConcurrentMap<long, messages::LazyLoadedImage *> Emotes::twitchEmoteFromCache;
ConcurrentMap<QString, messages::LazyLoadedImage *> Emotes::miscImageFromCache;

int Emotes::generation = 0;

Emotes::Emotes()
{
}

messages::LazyLoadedImage *
Emotes::getTwitchEmoteById(const QString &name, long id)
{
    return Emotes::twitchEmoteFromCache.getOrAdd(id, [&name, id] {
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

    return Emotes::twitchEmoteTemplate.replace("{id}", QString::number(id))
        .replace("{scale}", "2");
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
