#pragma once

#define GIF_FRAME_LENGTH 33

#include "concurrentmap.hpp"
#include "emojis.hpp"
#include "messages/lazyloadedimage.hpp"
#include "twitch/emotevalue.hpp"

#include <QMap>
#include <QMutex>
#include <QString>
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

    // Emojis
    // shortCodeToEmoji maps strings like ":sunglasses:" to the unicode character
    QMap<QString, EmojiData> shortCodeToEmoji;

    // emojiToShortCode maps the unicode character to the shortcode like ":sunglasses:"
    QMap<QString, QString> emojiToShortCode;

    // TODO(pajlada): Figure out what this is for
    QMap<QChar, QMap<QString, QString>> firstEmojiChars;

    ConcurrentMap<QString, messages::LazyLoadedImage *> emojis;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<messages::LazyLoadedImage *, QString>> &vector,
                     const QString &text);

private:
    // Twitch emotes
    ConcurrentMap<QString, twitch::EmoteValue *> _twitchEmotes;
    ConcurrentMap<long, messages::LazyLoadedImage *> _twitchEmoteFromCache;

    // BTTV emotes
    ConcurrentMap<QString, messages::LazyLoadedImage *> bttvChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _bttvChannelEmoteFromCaches;

    void loadBTTVEmotes();

    // FFZ emotes
    ConcurrentMap<QString, messages::LazyLoadedImage *> ffzChannelEmotes;
    ConcurrentMap<QString, messages::LazyLoadedImage *> _ffzEmotes;
    ConcurrentMap<int, messages::LazyLoadedImage *> _ffzChannelEmoteFromCaches;

    void loadFFZEmotes();

    // Chatterino emotes
    ConcurrentMap<QString, messages::LazyLoadedImage *> _chatterinoEmotes;

    // ???
    ConcurrentMap<QString, messages::LazyLoadedImage *> _miscImageFromCache;

    boost::signals2::signal<void()> _gifUpdateTimerSignal;
    QTimer _gifUpdateTimer;
    bool _gifUpdateTimerInitiated = false;

    int _generation = 0;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);
};

}  // namespace chatterino
