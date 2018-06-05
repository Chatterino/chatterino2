#include "providers/emoji/emojis.hpp"

namespace chatterino {
namespace providers {
namespace emoji {

void Emojis::load()
{
    QFile file(":/emojidata.txt");
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    uint unicodeBytes[4];

    while (!in.atEnd()) {
        // Line example: sunglasses 1f60e
        QString line = in.readLine();

        if (line.at(0) == '#') {
            // Ignore lines starting with # (comments)
            continue;
        }

        QStringList parts = line.split(' ');
        if (parts.length() < 2) {
            continue;
        }

        QString shortCode = parts[0];
        QString code = parts[1];

        QStringList unicodeCharacters = code.split('-');
        if (unicodeCharacters.length() < 1) {
            continue;
        }

        int numUnicodeBytes = 0;

        for (const QString &unicodeCharacter : unicodeCharacters) {
            unicodeBytes[numUnicodeBytes++] = QString(unicodeCharacter).toUInt(nullptr, 16);
        }

        QString unicodeString = QString::fromUcs4(unicodeBytes, numUnicodeBytes);

        QString url = "https://cdnjs.cloudflare.com/ajax/libs/"
                      "emojione/2.2.6/assets/png/" +
                      code + ".png";

        EmojiData emojiData{
            unicodeString,  //
            code,           //
            shortCode,      //
            {new messages::Image(url, 0.35, unicodeString, ":" + shortCode + ":<br/>Emoji")},
        };

        this->emojiShortCodeToEmoji.insert(shortCode, emojiData);
        this->shortCodes.push_back(shortCode.toStdString());

        this->emojiFirstByte[emojiData.value.at(0)].append(emojiData);

        this->emojis.insert(code, emojiData);
    }

    for (auto &p : this->emojiFirstByte) {
        std::stable_sort(p.begin(), p.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.value.length() > rhs.value.length();
        });
    }
}

void Emojis::parse(std::vector<std::tuple<util::EmoteData, QString>> &parsedWords,
                   const QString &text)
{
    int lastParsedEmojiEndIndex = 0;

    for (auto i = 0; i < text.length(); ++i) {
        const QChar character = text.at(i);

        if (character.isLowSurrogate()) {
            continue;
        }

        auto it = this->emojiFirstByte.find(character);
        if (it == this->emojiFirstByte.end()) {
            // No emoji starts with this character
            continue;
        }

        const QVector<EmojiData> possibleEmojis = it.value();

        int remainingCharacters = text.length() - i - 1;

        EmojiData matchedEmoji;

        int matchedEmojiLength = 0;

        for (const EmojiData &emoji : possibleEmojis) {
            int emojiExtraCharacters = emoji.value.length() - 1;
            if (emojiExtraCharacters > remainingCharacters) {
                // It cannot be this emoji, there's not enough space for it
                continue;
            }

            bool match = true;

            for (int j = 1; j < emoji.value.length(); ++j) {
                if (text.at(i + j) != emoji.value.at(j)) {
                    match = false;

                    break;
                }
            }

            if (match) {
                matchedEmoji = emoji;
                matchedEmojiLength = emoji.value.length();

                break;
            }
        }

        if (matchedEmojiLength == 0) {
            continue;
        }

        int currentParsedEmojiFirstIndex = i;
        int currentParsedEmojiEndIndex = i + (matchedEmojiLength);

        int charactersFromLastParsedEmoji = currentParsedEmojiFirstIndex - lastParsedEmojiEndIndex;

        if (charactersFromLastParsedEmoji > 0) {
            // Add characters inbetween emojis
            parsedWords.emplace_back(util::EmoteData(), text.mid(lastParsedEmojiEndIndex,
                                                                 charactersFromLastParsedEmoji));
        }

        // Push the emoji as a word to parsedWords
        parsedWords.push_back(
            std::tuple<util::EmoteData, QString>(matchedEmoji.emoteData, QString()));

        lastParsedEmojiEndIndex = currentParsedEmojiEndIndex;

        i += matchedEmojiLength - 1;
    }

    if (lastParsedEmojiEndIndex < text.length()) {
        // Add remaining characters
        parsedWords.emplace_back(util::EmoteData(), text.mid(lastParsedEmojiEndIndex));
    }
}

QString Emojis::replaceShortCodes(const QString &text)
{
    QString ret(text);
    auto it = this->findShortCodesRegex.globalMatch(text);

    int32_t offset = 0;

    while (it.hasNext()) {
        auto match = it.next();

        auto capturedString = match.captured();

        QString matchString = capturedString.toLower().mid(1, capturedString.size() - 2);

        auto emojiIt = this->emojiShortCodeToEmoji.constFind(matchString);

        if (emojiIt == this->emojiShortCodeToEmoji.constEnd()) {
            continue;
        }

        auto emojiData = emojiIt.value();

        ret.replace(offset + match.capturedStart(), match.capturedLength(), emojiData.value);

        offset += emojiData.value.size() - match.capturedLength();
    }

    return ret;
}

}  // namespace emoji
}  // namespace providers
}  // namespace chatterino
