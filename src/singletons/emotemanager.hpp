#pragma once

#define GIF_FRAME_LENGTH 33

#include "emojis.hpp"
#include "messages/image.hpp"
#include "providers/twitch/emotevalue.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "signalvector.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <QMap>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <boost/signals2.hpp>

namespace chatterino {
namespace singletons {

class SettingManager;
class WindowManager;

class EmoteManager
{
    explicit EmoteManager(singletons::SettingManager &manager,
                          singletons::WindowManager &windowManager);

public:
    static EmoteManager &getInstance();

    void loadGlobalEmotes();

    void reloadBTTVChannelEmotes(const QString &channelName,
                                 std::weak_ptr<util::EmoteMap> channelEmoteMap);
    void reloadFFZChannelEmotes(const QString &channelName,
                                std::weak_ptr<util::EmoteMap> channelEmoteMap);

    util::ConcurrentMap<QString, providers::twitch::EmoteValue *> &getTwitchEmotes();
    util::EmoteMap &getFFZEmotes();
    util::EmoteMap &getChatterinoEmotes();
    util::EmoteMap &getBTTVChannelEmoteFromCaches();
    util::EmoteMap &getEmojis();
    util::ConcurrentMap<int, util::EmoteData> &getFFZChannelEmoteFromCaches();
    util::ConcurrentMap<long, util::EmoteData> &getTwitchEmoteFromCache();

    util::EmoteData getCheerImage(long long int amount, bool animated);

    util::EmoteData getTwitchEmoteById(long int id, const QString &emoteName);

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
    util::ConcurrentMap<QString, messages::Image *> miscImageCache;

private:
    SettingManager &settingsManager;
    WindowManager &windowManager;

    /// Emojis
    QRegularExpression findShortCodesRegex;

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;

    //            url      Emoji-one image
    util::EmoteMap emojis;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords,
                     const QString &text);

    QString replaceShortCodes(const QString &text);

    std::vector<std::string> emojiShortCodes;

    /// Twitch emotes
    void refreshTwitchEmotes(const std::shared_ptr<providers::twitch::TwitchAccount> &user);

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
    util::ConcurrentMap<QString, providers::twitch::EmoteValue *> _twitchEmotes;

    //        emote id
    util::ConcurrentMap<long, util::EmoteData> _twitchEmoteFromCache;

    /// BTTV emotes
    util::EmoteMap bttvChannelEmotes;

public:
    util::ConcurrentMap<QString, util::EmoteMap> bttvChannels;
    util::EmoteMap bttvGlobalEmotes;
    SignalVector<std::string> bttvGlobalEmoteCodes;
    //       roomID
    std::map<std::string, SignalVector<std::string>> bttvChannelEmoteCodes;
    util::EmoteMap _bttvChannelEmoteFromCaches;

private:
    void loadBTTVEmotes();

    /// FFZ emotes
    util::EmoteMap ffzChannelEmotes;

public:
    util::ConcurrentMap<QString, util::EmoteMap> ffzChannels;
    util::EmoteMap ffzGlobalEmotes;
    SignalVector<std::string> ffzGlobalEmoteCodes;
    std::map<std::string, SignalVector<std::string>> ffzChannelEmoteCodes;

private:
    util::ConcurrentMap<int, util::EmoteData> _ffzChannelEmoteFromCaches;

    void loadFFZEmotes();

    /// Chatterino emotes
    util::EmoteMap _chatterinoEmotes;

    boost::signals2::signal<void()> gifUpdateTimerSignal;
    QTimer gifUpdateTimer;
    bool gifUpdateTimerInitiated = false;

    int _generation = 0;
};

}  // namespace singletons
}  // namespace chatterino
