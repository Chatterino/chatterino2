#ifndef EMOTES_H
#define EMOTES_H

#define GIF_FRAME_LENGTH 33

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"
#include "twitch/emotevalue.h"
#include "windowmanager.h"

#include <QMap>
#include <QMutex>
#include <QTimer>
#include <boost/signals2.hpp>

namespace chatterino {
class EmoteManager
{
public:
    static EmoteManager &getInstance()
    {
        return instance;
    }

    ConcurrentMap<QString, twitch::EmoteValue *> &getTwitchEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBttvEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getFfzEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getChatterinoEmotes();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getBttvChannelEmoteFromCaches();
    ConcurrentMap<int, messages::LazyLoadedImage *> &getFfzChannelEmoteFromCaches();
    ConcurrentMap<long, messages::LazyLoadedImage *> &getTwitchEmoteFromCache();
    ConcurrentMap<QString, messages::LazyLoadedImage *> &getMiscImageFromCache();

    void loadGlobalEmotes();

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

    boost::signals2::signal<void()> &getGifUpdateSignal()
    {
        if (!_gifUpdateTimerInitiated) {
            _gifUpdateTimerInitiated = true;

            _gifUpdateTimer.setInterval(30);
            _gifUpdateTimer.start();

            QObject::connect(&_gifUpdateTimer, &QTimer::timeout, [this] {
                _gifUpdateTimerSignal();
                WindowManager::getInstance().repaintGifEmotes();
            });
        }

        return _gifUpdateTimerSignal;
    }

private:
    static EmoteManager instance;

    EmoteManager();

    // variables
    ConcurrentMap<QString, twitch::EmoteValue *> _twitchEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _ffzEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _chatterinoEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvChannelEmoteFromCaches;
    ConcurrentMap<int, messages::LazyLoadedImage *> _ffzChannelEmoteFromCaches;
    ConcurrentMap<long, messages::LazyLoadedImage *> _twitchEmoteFromCache;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _miscImageFromCache;

    QTimer _gifUpdateTimer;
    bool _gifUpdateTimerInitiated;

    int _generation;

    boost::signals2::signal<void()> _gifUpdateTimerSignal;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);

    void loadFfzEmotes();
    void loadBttvEmotes();
};
}  // namespace chatterino

#endif  // EMOTES_H
