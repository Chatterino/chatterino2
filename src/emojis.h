#pragma once

#include "concurrentmap.h"
#include "messages/lazyloadedimage.h"

#include <QObject>
#include <QRegularExpression>
#include <QString>

#include <unordered_map>

namespace chatterino {

class Emojis
{
public:
    static void parseEmojis(std::vector<std::tuple<messages::LazyLoadedImage *, QString>> &vector,
                            const QString &text);

    static void loadEmojis();

    static QString replaceShortCodes(const QString &text);

    struct EmojiData {
        QString value;
        QString code;
    };

private:
    static QRegularExpression findShortCodesRegex;

    static QMap<QString, EmojiData> shortCodeToEmoji;
    static QMap<QString, QString> emojiToShortCode;
    static QMap<QChar, QMap<QString, QString>> firstEmojiChars;

    static ConcurrentMap<QString, messages::LazyLoadedImage *> imageCache;

    Emojis()
    {
    }
};

}  // namespace chatterino
