#pragma once

#define GIF_FRAME_LENGTH 33

#include "emojis.hpp"
#include "messages/image.hpp"
#include "providers/bttv/bttvemotes.hpp"
#include "providers/twitch/twitchemotes.hpp"
#include "signalvector.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <QMap>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {

class EmoteManager
{
public:
    EmoteManager();

    ~EmoteManager() = delete;

    providers::twitch::TwitchEmotes twitch;
    providers::bttv::BTTVEmotes bttv;

    void initialize();

    void reloadFFZChannelEmotes(const QString &channelName,
                                std::weak_ptr<util::EmoteMap> channelEmoteMap);

    util::EmoteMap &getFFZEmotes();
    util::EmoteMap &getChatterinoEmotes();
    util::EmojiMap &getEmojis();
    util::ConcurrentMap<int, util::EmoteData> &getFFZChannelEmoteFromCaches();

    util::EmoteData getCheerImage(long long int amount, bool animated);

    pajlada::Signals::NoArgSignal &getGifUpdateSignal();

    // Bit badge/emotes?
    util::ConcurrentMap<QString, messages::Image *> miscImageCache;

private:
    /// Emojis
    QRegularExpression findShortCodesRegex;

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;

    util::EmojiMap emojis;

    void loadEmojis();

public:
    void parseEmojis(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords,
                     const QString &text);

    QString replaceShortCodes(const QString &text);

    std::vector<std::string> emojiShortCodes;

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

    pajlada::Signals::NoArgSignal gifUpdateTimerSignal;
    QTimer gifUpdateTimer;
    bool gifUpdateTimerInitiated = false;
};

}  // namespace singletons
}  // namespace chatterino
