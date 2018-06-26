#pragma once

#include "common/SignalVector.hpp"
#include "util/ConcurrentMap.hpp"
#include "common/Emotemap.hpp"

#include <QMap>
#include <QRegularExpression>

#include <map>

namespace chatterino {

struct EmojiData {
    // actual byte-representation of the emoji (i.e. \154075\156150 which is :male:)
    QString value;

    // i.e. 204e-50a2
    QString unifiedCode;
    QString nonQualifiedCode;

    // i.e. thinking
    std::vector<QString> shortCodes;

    std::set<QString> capabilities;

    std::vector<EmojiData> variations;

    EmoteData emoteData;
};

using EmojiMap = ConcurrentMap<QString, std::shared_ptr<EmojiData>>;

class Emojis
{
public:
    void initialize();

    EmojiMap emojis;

    std::vector<QString> shortCodes;

    void load();

private:
    void loadEmojis();
    void loadEmojiOne2Capabilities();
    void sortEmojis();
    void loadEmojiSet();

public:
    QString replaceShortCodes(const QString &text);

    void parse(std::vector<std::tuple<EmoteData, QString>> &parsedWords, const QString &text);

private:
    /// Emojis
    QRegularExpression findShortCodesRegex{":([-+\\w]+):"};

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, std::shared_ptr<EmojiData>> emojiShortCodeToEmoji;

    // Maps the first character of the emoji unicode string to a vector of possible emojis
    QMap<QChar, QVector<std::shared_ptr<EmojiData>>> emojiFirstByte;
};

}  // namespace chatterino
