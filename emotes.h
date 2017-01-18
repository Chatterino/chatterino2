#ifndef EMOTES_H
#define EMOTES_H

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"
#include "twitchemotevalue.h"

#include <QMap>
#include <QMutex>

namespace chatterino {

class Emotes
{
public:
    static ConcurrentMap<QString, TwitchEmoteValue *> &
    getTwitchEmotes()
    {
        return twitchEmotes;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getBttvEmotes()
    {
        return bttvEmotes;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getFfzEmotes()
    {
        return ffzEmotes;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getChatterinoEmotes()
    {
        return chatterinoEmotes;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getBttvChannelEmoteFromCaches()
    {
        return bttvChannelEmoteFromCaches;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getFfzChannelEmoteFromCaches()
    {
        return ffzChannelEmoteFromCaches;
    }

    static ConcurrentMap<long, messages::LazyLoadedImage *> &
    getTwitchEmoteFromCache()
    {
        return twitchEmoteFromCache;
    }

    static ConcurrentMap<QString, messages::LazyLoadedImage *> &
    getMiscImageFromCache()
    {
        return miscImageFromCache;
    }

    static void loadGlobalEmotes();

    static messages::LazyLoadedImage *getCheerImage(long long int amount,
                                                    bool animated);
    static messages::LazyLoadedImage *getCheerBadge(long long int amount);

    static messages::LazyLoadedImage *getTwitchEmoteById(const QString &name,
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
    static ConcurrentMap<QString, messages::LazyLoadedImage *> bttvEmotes;
    static ConcurrentMap<QString, messages::LazyLoadedImage *> ffzEmotes;
    static ConcurrentMap<QString, messages::LazyLoadedImage *> chatterinoEmotes;
    static ConcurrentMap<QString, messages::LazyLoadedImage *>
        bttvChannelEmoteFromCaches;
    static ConcurrentMap<QString, messages::LazyLoadedImage *>
        ffzChannelEmoteFromCaches;
    static ConcurrentMap<long, messages::LazyLoadedImage *>
        twitchEmoteFromCache;
    static ConcurrentMap<QString, messages::LazyLoadedImage *>
        miscImageFromCache;

    static QString getTwitchEmoteLink(long id, qreal &scale);

    static int generation;
};
}

#endif  // EMOTES_H
