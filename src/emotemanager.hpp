#pragma once

#define GIF_FRAME_LENGTH 33

#include "concurrentmap.hpp"
#include "emojis.hpp"
#include "messages/lazyloadedimage.hpp"
#include "signalvector.hpp"
#include "twitch/emotevalue.hpp"

#include <QMap>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <boost/signals2.hpp>

namespace chatterino {

class WindowManager;

class Emote
{
public:
    Emote() = default;

    /**
     * @param emoteName name of the emote (what chatter writes in chat)
     * @param emoteChannel name of the channel which possesses this emote
     * @param emoteCreator name of the user (FFZ / BTTV) who made this emote
     */
    Emote(const QString &emoteName, const QString &emoteUrl, const QString &emoteChannel = "",
          const QString &emoteCreator = "");

    QString name;
    QString url;
    QString channel;
    QString creator;
};

typedef ConcurrentMap<QString, Emote> EmoteMap;

class EmoteManager
{
public:
    explicit EmoteManager(WindowManager &_windowManager);

    void loadGlobalEmotes();

    void reloadBTTVChannelEmotes(const QString &channelName,
                                 std::weak_ptr<EmoteMap> channelEmoteMap);
    void reloadFFZChannelEmotes(const QString &channelName,
                                std::weak_ptr<EmoteMap> channelEmoteMap);

    ConcurrentMap<QString, twitch::EmoteValue *> &getTwitchEmotes();
    EmoteMap &getFFZEmotes();
    EmoteMap &getChatterinoEmotes();
    EmoteMap &getEmojis();

    Emote getCheerImage(long long int amount, bool animated);

    Emote getTwitchEmoteById(long int id, const QString &emoteName);

    int getGeneration()
    {
        return _generation;
    }

    void incGeneration()
    {
        _generation++;
    }

    // Bit badge/emotes?
    ConcurrentMap<QString, messages::LazyLoadedImage *> miscImageCache;

private:
    WindowManager &windowManager;

    /// Emojis
    QRegularExpression findShortCodesRegex;

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;

    //            url      Emoji-one image
    EmoteMap emojiCache;
    EmoteMap emojis;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<std::unique_ptr<Emote>, QString>> &parsedWords,
                     const QString &text);

    QString replaceShortCodes(const QString &text);

    std::vector<std::string> emojiShortCodes;

    /// Twitch emotes
    void refreshTwitchEmotes(const std::string &roomID);

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
    void loadFFZEmotes();

    /// Chatterino emotes
    EmoteMap _chatterinoEmotes;

    boost::signals2::signal<void()> _gifUpdateTimerSignal;
    QTimer _gifUpdateTimer;

    int _generation = 0;

    // methods
    static QString getTwitchEmoteLink(long id, qreal &scale);
};

}  // namespace chatterino
