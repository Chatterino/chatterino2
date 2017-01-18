#ifndef EMOTES_H
#define EMOTES_H

#include "concurrentmap.h"
#include "lazyloadedimage.h"
#include "twitchemotevalue.h"

#include <QMap>
#include <QMutex>

class Emotes
{
public:
    static ConcurrentMap<QString, TwitchEmoteValue *> &
    getTwitchEmotes()
    {
        return twitchEmotes;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getBttvEmotes()
    {
        return bttvEmotes;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getFfzEmotes()
    {
        return ffzEmotes;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getChatterinoEmotes()
    {
        return chatterinoEmotes;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getBttvChannelEmoteFromCaches()
    {
        return bttvChannelEmoteFromCaches;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getFfzChannelEmoteFromCaches()
    {
        return ffzChannelEmoteFromCaches;
    }

    static ConcurrentMap<long, LazyLoadedImage *> &
    getTwitchEmoteFromCache()
    {
        return twitchEmoteFromCache;
    }

    static ConcurrentMap<QString, LazyLoadedImage *> &
    getMiscImageFromCache()
    {
        return miscImageFromCache;
    }

    static void loadGlobalEmotes();

    static LazyLoadedImage *getCheerImage(long long int amount, bool animated);
    static LazyLoadedImage *getCheerBadge(long long int amount);

    static LazyLoadedImage *getTwitchEmoteById(const QString &name,
                                               long int id);

    static int
    getGeneration()
    {
        return generation;
    }

    static void
    incGeneration()
    {
        generation++;
    }

private:
    Emotes();

    static QString twitchEmoteTemplate;

    static ConcurrentMap<QString, TwitchEmoteValue *> twitchEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> bttvEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> ffzEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> chatterinoEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> bttvChannelEmoteFromCaches;
    static ConcurrentMap<QString, LazyLoadedImage *> ffzChannelEmoteFromCaches;
    static ConcurrentMap<long, LazyLoadedImage *> twitchEmoteFromCache;
    static ConcurrentMap<QString, LazyLoadedImage *> miscImageFromCache;

    static QString getTwitchEmoteLink(long id, qreal &scale);

    static int generation;
};

#endif  // EMOTES_H
