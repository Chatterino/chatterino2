#pragma once

#include "signalvector.hpp"
#include "util/concurrentmap.hpp"
#include "util/emotemap.hpp"

#include <QMap>
#include <QRegularExpression>

#include <map>

namespace chatterino {
namespace providers {
namespace emoji {

struct EmojiData {
    // actual byte-representation of the emoji (i.e. \154075\156150 which is :male:)
    QString value;

    // what's used in the emoji-one url
    QString code;

    // i.e. thinking
    QString shortCode;

    util::EmoteData emoteData;
};

using EmojiMap = util::ConcurrentMap<QString, EmojiData>;

class Emojis
{
public:
    EmojiMap emojis;

    std::vector<std::string> shortCodes;

    void load();
    QString replaceShortCodes(const QString &text);

    void parse(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords, const QString &text);

private:
    /// Emojis
    QRegularExpression findShortCodesRegex{":([-+\\w]+):"};

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, EmojiData> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<EmojiData>> emojiFirstByte;
};

}  // namespace emoji
}  // namespace providers
}  // namespace chatterino
