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

    // i.e. 204e-50a2
    QString unifiedCode;
    QString nonQualifiedCode;

    // i.e. thinking
    QString shortCode;

    std::set<QString> capabilities;

    std::vector<EmojiData> variations;

    util::EmoteData emoteData;
};

using EmojiMap = util::ConcurrentMap<QString, std::shared_ptr<EmojiData>>;

class Emojis
{
public:
    void initialize();

    EmojiMap emojis;

    std::vector<std::string> shortCodes;

    void load();

private:
    void loadEmojis();
    void loadEmojiOne2Capabilities();

public:
    QString replaceShortCodes(const QString &text);

    void parse(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords, const QString &text);

private:
    /// Emojis
    QRegularExpression findShortCodesRegex{":([-+\\w]+):"};

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, std::shared_ptr<EmojiData>> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<std::shared_ptr<EmojiData>>> emojiFirstByte;
};

}  // namespace emoji
}  // namespace providers
}  // namespace chatterino
