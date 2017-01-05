#ifndef EMOTES_H
#define EMOTES_H

#include "twitchemotevalue.h"
#include "lazyloadedimage.h"
#include "QMutex"
#include "QMap"
#include "concurrentmap.h"

class Emotes
{
public:
    static ConcurrentMap<QString, TwitchEmoteValue*>& twitchEmotes()               { return *m_twitchEmotes              ; }
    static ConcurrentMap<QString, LazyLoadedImage* >& bttvEmotes()                 { return *m_bttvEmotes                ; }
    static ConcurrentMap<QString, LazyLoadedImage* >& ffzEmotes()                  { return *m_ffzEmotes                 ; }
    static ConcurrentMap<QString, LazyLoadedImage* >& chatterinoEmotes()           { return *m_chatterinoEmotes          ; }
    static ConcurrentMap<QString, LazyLoadedImage* >& bttvChannelEmoteFromCaches() { return *m_bttvChannelEmoteFromCaches; }
    static ConcurrentMap<QString, LazyLoadedImage* >& fFzChannelEmoteFromCaches()  { return *m_fFzChannelEmoteFromCaches ; }
    static ConcurrentMap<int,     LazyLoadedImage* >& twitchEmoteFromCache()       { return *m_twitchEmoteFromCache      ; }
    static ConcurrentMap<int,     LazyLoadedImage* >& miscImageFromCache()         { return *m_miscImageFromCache        ; }

    static void loadGlobalEmotes();

    static LazyLoadedImage* getCheerImage(long long int amount, bool animated);

private:
    Emotes();

    static ConcurrentMap<QString, TwitchEmoteValue*>* m_twitchEmotes;
    static ConcurrentMap<QString, LazyLoadedImage* >* m_bttvEmotes;
    static ConcurrentMap<QString, LazyLoadedImage* >* m_ffzEmotes;
    static ConcurrentMap<QString, LazyLoadedImage* >* m_chatterinoEmotes;
    static ConcurrentMap<QString, LazyLoadedImage* >* m_bttvChannelEmoteFromCaches;
    static ConcurrentMap<QString, LazyLoadedImage* >* m_fFzChannelEmoteFromCaches;
    static ConcurrentMap<int,     LazyLoadedImage* >* m_twitchEmoteFromCache;
    static ConcurrentMap<int,     LazyLoadedImage* >* m_miscImageFromCache;
};

#endif // EMOTES_H
