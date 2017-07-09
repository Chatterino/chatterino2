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

struct EmoteData {
    EmoteData()
    {
    }

    EmoteData(messages::LazyLoadedImage *_image)
        : image(_image)
    {
    }

    messages::LazyLoadedImage *image = nullptr;
};

class EmoteManager
{
public:
    using EmoteMap = ConcurrentMap<QString, EmoteData>;

    EmoteManager(WindowManager &_windowManager, Resources &_resources);

    void loadGlobalEmotes();

    void reloadBTTVChannelEmotes(const QString &channelName);
    void reloadFFZChannelEmotes(const QString &channelName);

    ConcurrentMap<QString, twitch::EmoteValue *> &getTwitchEmotes();
    EmoteMap &getBTTVEmotes();
    EmoteMap &getFFZEmotes();
    EmoteMap &getChatterinoEmotes();
    EmoteMap &getBTTVChannelEmoteFromCaches();
    ConcurrentMap<int, EmoteData> &getFFZChannelEmoteFromCaches();
    ConcurrentMap<long, EmoteData> &getTwitchEmoteFromCache();

    EmoteData getCheerImage(long long int amount, bool animated);

    EmoteData getTwitchEmoteById(long int id, const QString &emoteName);

    int getGeneration()
    {
        return _generation;
    }

    void incGeneration()
    {
        _generation++;
    }

    boost::signals2::signal<void()> &getGifUpdateSignal();

    // Bit badge/emotes?
    ConcurrentMap<QString, messages::LazyLoadedImage *> miscImageCache;

private:
    WindowManager &windowManager;
    Resources &resources;

    /// Emojis
    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;

    //            url      Emoji-one image
    EmoteMap emojiCache;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<EmoteData, QString>> &parsedWords, const QString &text);

private:
    /// Twitch emotes
    ConcurrentMap<QString, twitch::EmoteValue *> _twitchEmotes;
    ConcurrentMap<long, EmoteData> _twitchEmoteFromCache;

    /// BTTV emotes
    EmoteMap bttvChannelEmotes;

public:
    ConcurrentMap<QString, EmoteMap> bttvChannels;

private:
    EmoteMap _bttvEmotes;
    EmoteMap _bttvChannelEmoteFromCaches;

    void loadBTTVEmotes();

    /// FFZ emotes
    EmoteMap ffzChannelEmotes;

public:
    ConcurrentMap<QString, EmoteMap> ffzChannels;

private:
    EmoteMap _ffzEmotes;
    ConcurrentMap<int, EmoteData> _ffzChannelEmoteFromCaches;

    void loadFFZEmotes();

    /// Chatterino emotes
    EmoteMap _chatterinoEmotes;

    boost::signals2::signal<void()> _gifUpdateTimerSignal;
    QTimer _gifUpdateTimer;
    bool _gifUpdateTimerInitiated = false;

    int _generation = 0;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);
};

}  // namespace chatterino
