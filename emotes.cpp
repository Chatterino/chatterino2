#include "emotes.h"

ConcurrentMap<QString, TwitchEmoteValue*>* Emotes::m_twitchEmotes               = new ConcurrentMap<QString, TwitchEmoteValue*>();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvEmotes                 = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_ffzEmotes                  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_chatterinoEmotes           = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_bttvChannelEmoteFromCaches = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<QString, LazyLoadedImage* >* Emotes::m_fFzChannelEmoteFromCaches  = new ConcurrentMap<QString, LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_twitchEmoteFromCache       = new ConcurrentMap<int,     LazyLoadedImage* >();
ConcurrentMap<int,     LazyLoadedImage* >* Emotes::m_miscImageFromCache         = new ConcurrentMap<int,     LazyLoadedImage* >();

Emotes::Emotes()
{

}

LazyLoadedImage* Emotes::getCheerImage(long long amount, bool animated)
{
#warning "xD"
//    object image;

//    if (cheer >= 100000)
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer100000);
//    }
//    else if (cheer >= 10000)
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer10000);
//    }
//    else if (cheer >= 5000)
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer5000);
//    }
//    else if (cheer >= 1000)
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer1000);
//    }
//    else if (cheer >= 100)
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer100);
//    }
//    else
//    {
//        image = GuiEngine.Current.GetImage(ImageType.Cheer1);
//    }

//    words.Add(new Word { Type = SpanType.Image, Value = image, Tooltip = "Twitch Cheer " + cheer });

    return new LazyLoadedImage("");
}
