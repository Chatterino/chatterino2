#ifndef EMOTES_H
#define EMOTES_H

#include "QMap"
#include "QMutex"
#include "concurrentmap.h"
#include "lazyloadedimage.h"
#include "twitchemotevalue.h"

class Emotes
{
public:
    static ConcurrentMap<QString, TwitchEmoteValue *> &
    twitchEmotes()
    {
        return *m_twitchEmotes;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    bttvEmotes()
    {
        return *m_bttvEmotes;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    ffzEmotes()
    {
        return *m_ffzEmotes;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    chatterinoEmotes()
    {
        return *m_chatterinoEmotes;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    bttvChannelEmoteFromCaches()
    {
        return *m_bttvChannelEmoteFromCaches;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    ffzChannelEmoteFromCaches()
    {
        return *m_ffzChannelEmoteFromCaches;
    }
    static ConcurrentMap<int, LazyLoadedImage *> &
    twitchEmoteFromCache()
    {
        return *m_twitchEmoteFromCache;
    }
    static ConcurrentMap<QString, LazyLoadedImage *> &
    miscImageFromCache()
    {
        return *m_miscImageFromCache;
    }

    static void loadGlobalEmotes();

    static LazyLoadedImage *getCheerImage(long long int amount, bool animated);
    static LazyLoadedImage *getCheerBadge(long long int amount);

    static LazyLoadedImage *getTwitchEmoteById(const QString &name,
                                               long int id);

private:
    Emotes();

    static ConcurrentMap<QString, TwitchEmoteValue *> *m_twitchEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> *m_bttvEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> *m_ffzEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *> *m_chatterinoEmotes;
    static ConcurrentMap<QString, LazyLoadedImage *>
        *m_bttvChannelEmoteFromCaches;
    static ConcurrentMap<QString, LazyLoadedImage *>
        *m_ffzChannelEmoteFromCaches;
    static ConcurrentMap<int, LazyLoadedImage *> *m_twitchEmoteFromCache;
    static ConcurrentMap<QString, LazyLoadedImage *> *m_miscImageFromCache;

    static LazyLoadedImage *m_cheerBadge100000;
    static LazyLoadedImage *m_cheerBadge10000;
    static LazyLoadedImage *m_cheerBadge5000;
    static LazyLoadedImage *m_cheerBadge1000;
    static LazyLoadedImage *m_cheerBadge100;
    static LazyLoadedImage *m_cheerBadge1;
};

#endif  // EMOTES_H
