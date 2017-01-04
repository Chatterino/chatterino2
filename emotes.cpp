#include "emotes.h"

ConcurrentMap<QString, TwitchEmoteValue*>* Emotes::m_twitchEmotes               = new ConcurrentMap<QString, TwitchEmoteValue*>();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvEmotes                 = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_ffzEmotes                  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_chatterinoEmotes           = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvChannelEmoteFromCaches = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_fFzChannelEmoteFromCaches  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_twitchEmoteFromCache       = new ConcurrentMap<int,     LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_miscImageFromCache         = new ConcurrentMap<int,     LazyLoadedImage* >();

//QMutex* Emotes::mutexBttvEmote = new QMutex();
//QMap<QString, LazyLoadedImage*>* Emotes::mapBttvEmote = new QMap<QString, LazyLoadedImage*>();

//LazyLoadedImage* Emotes::getBttvEmote(const QString &name) {
//    mutexBttvEmote->lock();
//    auto a = mapBttvEmote->find(name);
//    if (a == mapBttvEmote->end()) {
//        mutexBttvEmote->unlock();
//        return NULL;
//    }
//    mutexBttvEmote->unlock();
//    return a.value();
//}

//void


Emotes::Emotes()
{

}
