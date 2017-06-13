#pragma once

#define GIF_FRAME_LENGTH 33

#include "concurrentmap.hpp"
#include "messages/lazyloadedimage.hpp"
#include "twitch/emotevalue.hpp"

#include <QMap>
#include <QMutex>
#include <QTimer>
#include <boost/signals2.hpp>

namespace chatterino {

class WindowManager;
class Resources;

class EmoteManager
{
public:
    using FFZEmoteMap = ConcurrentMap<QString, messages::LazyLoadedImage *>;
    using BTTVEmoteMap = ConcurrentMap<QString, messages::LazyLoadedImage *>;
    using ChatterinoEmoteMap = ConcurrentMap<QString, messages::LazyLoadedImage *>;

    EmoteManager(WindowManager &_windowManager, Resources &_resources);

    void loadGlobalEmotes();

    void reloadBTTVChannelEmotes(const QString &channelName, BTTVEmoteMap &channelEmoteMap);
    void reloadFFZChannelEmotes(const QString &channelName, FFZEmoteMap &channelEmoteMap);

    ConcurrentMap<QString, twitch::EmoteValue *> &getTwitchEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBTTVEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getFFZEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getChatterinoEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBTTVChannelEmoteFromCaches();
    ConcurrentMap<int, messages::LazyLoadedImage *> &getFFZChannelEmoteFromCaches();
    ConcurrentMap<long, messages::LazyLoadedImage *> &getTwitchEmoteFromCache();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getMiscImageFromCache();

    messages::LazyLoadedImage *getCheerImage(long long int amount, bool animated);
    messages::LazyLoadedImage *getCheerBadge(long long int amount);

    messages::LazyLoadedImage *getTwitchEmoteById(const QString &name, long int id);

    int getGeneration()
    {
        return _generation;
    }

    void incGeneration()
    {
        _generation++;
    }

    boost::signals2::signal<void()> &getGifUpdateSignal();

private:
    WindowManager &windowManager;
    Resources &resources;

    ConcurrentMap<QString, messages::LazyLoadedImage *> bttvChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> ffzChannelEmotes;

    ConcurrentMap<QString, twitch::EmoteValue *> _twitchEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _ffzEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _chatterinoEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvChannelEmoteFromCaches;
    ConcurrentMap<int, messages::LazyLoadedImage *> _ffzChannelEmoteFromCaches;
    ConcurrentMap<long, messages::LazyLoadedImage *> _twitchEmoteFromCache;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _miscImageFromCache;

    boost::signals2::signal<void()> _gifUpdateTimerSignal;
    QTimer _gifUpdateTimer;
    bool _gifUpdateTimerInitiated = false;

    int _generation = 0;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);

    void loadFFZEmotes();
    void loadBTTVEmotes();
};

}  // namespace chatterino
