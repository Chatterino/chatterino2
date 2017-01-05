#include "emotes.h"

ConcurrentMap<QString, TwitchEmoteValue*>* Emotes::m_twitchEmotes               = new ConcurrentMap<QString, TwitchEmoteValue*>();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvEmotes                 = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_ffzEmotes                  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_chatterinoEmotes           = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvChannelEmoteFromCaches = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_fFzChannelEmoteFromCaches  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_twitchEmoteFromCache       = new ConcurrentMap<int,     LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_miscImageFromCache         = new ConcurrentMap<int,     LazyLoadedImage* >();

LazyLoadedImage* Emotes::m_cheerBadge100000 = new LazyLoadedImage(new QImage(":/cheer100000"));
LazyLoadedImage* Emotes::m_cheerBadge10000  = new LazyLoadedImage(new QImage(":/cheer10000"));
LazyLoadedImage* Emotes::m_cheerBadge5000   = new LazyLoadedImage(new QImage(":/cheer5000"));
LazyLoadedImage* Emotes::m_cheerBadge1000   = new LazyLoadedImage(new QImage(":/cheer1000"));
LazyLoadedImage* Emotes::m_cheerBadge100    = new LazyLoadedImage(new QImage(":/cheer100"));
LazyLoadedImage* Emotes::m_cheerBadge1      = new LazyLoadedImage(new QImage(":/cheer1"));

Emotes::Emotes()
{

}

LazyLoadedImage* Emotes::getCheerImage(long long amount, bool animated)
{
#warning "xD"
    return getCheerBadge(amount);
}

LazyLoadedImage* Emotes::getCheerBadge(long long amount)
{
    if (amount >= 100000)
    {
        return m_cheerBadge100000;
    }
    else if (amount >= 10000)
    {
        return m_cheerBadge10000;
    }
    else if (amount >= 5000)
    {\
        return m_cheerBadge5000;
    }
    else if (amount >= 1000)
    {
        return m_cheerBadge1000;
    }
    else if (amount >= 100)
    {
        return m_cheerBadge100;
    }
    else
    {
        return m_cheerBadge1;
    }
}
