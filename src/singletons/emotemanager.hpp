#pragma once

#define GIF_FRAME_LENGTH 33

#include "concurrentmap.hpp"
#include "emojis.hpp"
#include "messages/lazyloadedimage.hpp"
#include "signalvector.hpp"
#include "twitch/emotevalue.hpp"
#include "twitch/twitchuser.hpp"

#include <QMap>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <boost/signals2.hpp>

namespace chatterino {

class SettingsManager;
class WindowManager;

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

typedef ConcurrentMap<QString, EmoteData> EmoteMap;

class EmoteManager
{
    explicit EmoteManager(SettingsManager &manager, WindowManager &windowManager);

public:
    static EmoteManager &getInstance();

    void loadGlobalEmotes();

    void reloadBTTVChannelEmotes(const QString &channelName,
                                 std::weak_ptr<EmoteMap> channelEmoteMap);
    void reloadFFZChannelEmotes(const QString &channelName,
                                std::weak_ptr<EmoteMap> channelEmoteMap);

    ConcurrentMap<QString, twitch::EmoteValue *> &getTwitchEmotes();
    EmoteMap &getFFZEmotes();
    EmoteMap &getChatterinoEmotes();
    EmoteMap &getBTTVChannelEmoteFromCaches();
    EmoteMap &getEmojis();
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
    SettingsManager &settingsManager;
    WindowManager &windowManager;

    /// Emojis
    QRegularExpression findShortCodesRegex;

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;

    //            url      Emoji-one image
    EmoteMap emojis;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<EmoteData, QString>> &parsedWords, const QString &text);

    QString replaceShortCodes(const QString &text);

    std::vector<std::string> emojiShortCodes;

    /// Twitch emotes
    void refreshTwitchEmotes(const std::shared_ptr<twitch::TwitchUser> &user);

    struct TwitchAccountEmoteData {
        struct TwitchEmote {
            std::string id;
            std::string code;
        };

        //       emote set
        std::map<std::string, std::vector<TwitchEmote>> emoteSets;

        std::vector<std::string> emoteCodes;

        bool filled = false;
    };

    std::map<std::string, TwitchAccountEmoteData> twitchAccountEmotes;

private:
    //            emote code
    ConcurrentMap<QString, twitch::EmoteValue *> _twitchEmotes;

    //        emote id
    ConcurrentMap<long, EmoteData> _twitchEmoteFromCache;

    /// BTTV emotes
    EmoteMap bttvChannelEmotes;

public:
    ConcurrentMap<QString, EmoteMap> bttvChannels;
    EmoteMap bttvGlobalEmotes;
    SignalVector<std::string> bttvGlobalEmoteCodes;
    //       roomID
    std::map<std::string, SignalVector<std::string>> bttvChannelEmoteCodes;
    EmoteMap _bttvChannelEmoteFromCaches;

private:
    void loadBTTVEmotes();

    /// FFZ emotes
    EmoteMap ffzChannelEmotes;

public:
    ConcurrentMap<QString, EmoteMap> ffzChannels;
    EmoteMap ffzGlobalEmotes;
    SignalVector<std::string> ffzGlobalEmoteCodes;
    std::map<std::string, SignalVector<std::string>> ffzChannelEmoteCodes;

private:
    ConcurrentMap<int, EmoteData> _ffzChannelEmoteFromCaches;

    void loadFFZEmotes();

    /// Chatterino emotes
    EmoteMap _chatterinoEmotes;

    boost::signals2::signal<void()> gifUpdateTimerSignal;
    QTimer gifUpdateTimer;
    bool gifUpdateTimerInitiated = false;

    int _generation = 0;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);
};

}  // namespace chatterino
